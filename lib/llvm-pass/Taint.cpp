
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include </usr/local/json/single_include/nlohmann/json.hpp>
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/IR/InstVisitor.h"
#include <fstream>
#include <list>
#include <set>
using namespace llvm;

#define DEBUG_TYPE "taint"
using json = nlohmann::json;

namespace FunctionTainter
{
  json j;
  void openJson()
  {
    std::ifstream tainted_functions(
        "tainted_functions.json");
    j = json::parse(tainted_functions);
  }
  void markFunction(std::string fileName, std::string functionName, int line)
  {
    errs() << fileName << " " << functionName << " " << line << '\n';
    j[fileName][functionName]["line"] = line;
  }
  void closeJson()
  {
    std::string fileName = "tainted_functions.json";
    std::ofstream file(fileName, std::ofstream::out);
    file << j.dump(2) << std::endl;
    file.close();
  }

} // namespace FunctionTainter
namespace utils
{
  Instruction *findLocation(Instruction *I, int line)
  {
    int new_line;
    do
    {
      if (DILocation *Loc = I->getDebugLoc())
      {
        new_line = Loc->getLine();
      }
      if (!I->getNextNode())
      {
        return I;
      }
      I = I->getNextNode();
    } while (line == new_line);
    return I->getPrevNode();
  }

  FunctionCallee getFunctionCallee(Module *M, Type *arg_types[], Value *args[],
                                   int len, Type *ret_type, std::string name)
  {
    std::vector<llvm::Type *> func_args_array;
    for (int i = 0; i < len; i++)
    {
      func_args_array.push_back(arg_types[i]);
    }
    ArrayRef<Type *> func_args_array_ref(func_args_array);
    FunctionType *function_type =
        FunctionType::get(ret_type, func_args_array_ref, false);
    FunctionCallee callee =
        M->getOrInsertFunction(name, function_type);
    return callee;
  }

} // namespace utils

namespace
{
  struct SetLabels : public ModulePass
  {
    static char ID;
    SetLabels() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
      errs() << "Step 1: Set labels of changed variables" << '\n';
      std::ifstream changes(
          "changes.json");
      json jf = json::parse(changes);
      std::string file = M.getSourceFileName();
      //errs() << file << '\n';
      // Set all labels
      if (jf.contains(file))
      {
        //errs() << file << '\n';
        for (auto curFref = M.getFunctionList().begin(),
                  endFref = M.getFunctionList().end();
             curFref != endFref; ++curFref)
        {
          StringRef fNameRef = curFref->getName();
          std::string fName = fNameRef.str();
          Function *F = M.getFunction(fNameRef);
          Metadata *meta = F->getMetadata("dbg");
          if (!meta)
          {
            continue;
          }
          if (DISubprogram *SP = dyn_cast<DISubprogram>(meta))
          {
            std::string name = SP->getName().str();
            if (jf[file].contains(name))
            {
              if (jf[file][name]["line"] == SP->getLine())
              {
                errs() << "Changes detected in " << file << " in function " << name
                       << '\n';
                setLabels(*F, jf[file][name]["variables"]);
              }
            }
          }
        }
      }
      return true;
    }

    bool setLabels(Function &F, json changes)
    {
      LLVMContext &context = F.getContext();
      IRBuilder<> builder(context);
      std::map<std::string, Value *> declarations;
      std::map<std::string, AllocaInst *> label_allocations;

      builder.SetInsertPoint(&(F.getEntryBlock().front()));

      int len = changes.size();
      for (auto &el : changes)
      {
        int line = el["line"];
        std::string label = el["label"];
        AllocaInst *alloca = builder.CreateAlloca(llvm::Type::getInt16Ty(context),
                                                  nullptr, label + "_label");
        label_allocations[el["label"]] = alloca;
      }

      // Instruction *insertPoint =
      //     utils::findLocation(&(F.getEntryBlock().front()), 0);
      // builder.SetInsertPoint(insertPoint);

      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      {
        if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(&*I))
        {
          if (MetadataAsValue *VAM =
                  dyn_cast<MetadataAsValue>(I->getOperand(0)))
          {
            Metadata *M = VAM->getMetadata();
            if (LocalAsMetadata *LAM = dyn_cast<LocalAsMetadata>(M))
            {
              std::string name = DbgDeclare->getVariable()->getName().str();
              for (auto &el : changes)
              {
                if (el["label"] == name && !declarations[name])
                {
                  declarations[name] = LAM->getValue();
                  Type *types[] = {Type::getInt8PtrTy(context)};
                  builder.SetInsertPoint(I->getNextNode());
                  Value *name_val = builder.CreateGlobalStringPtr(name);
                  Value *args[] = {name_val};
                  Type *ret_type = Type::getInt16Ty(context);
                  FunctionCallee c = utils::getFunctionCallee(
                      F.getParent(), types, args, 1, ret_type, "dfsan_create_label");
                  auto *label = builder.CreateCall(c, args);
                  builder.CreateStore(label, label_allocations[name]);
                  errs() << "create label " << name << '\n';
                  break;
                }
              }
            }
          }
        }
      }

      // for(std::map<std::string, AllocaInst *>::const_iterator it = label_allocations.begin(); it != label_allocations.end(); ++it ){
      //   std::string name = it->first;

      // }

      int new_line;
      int old_line;
      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      {
        if (DILocation *Loc = I->getDebugLoc())
        {
          new_line = Loc->getLine();
          if (old_line == new_line)
          {
            continue;
          }
          for (auto &el : changes)
          {
            // std::vector<int> lineNumbers = el.value();
            // int len = lineNumbers.size();
            // for (int i = 0; i < len; i++)
            // {

            int lineNumber = el["line"];
            if (lineNumber == new_line)
            {
              std::string labelName = el["label"];
              // Read label
              Instruction *insertPoint = utils::findLocation(&*I, new_line);

              builder.SetInsertPoint(insertPoint);
              auto *cast = builder.CreateBitCast(
                  declarations[labelName], llvm::Type::getInt8PtrTy(context));

              // Load initial label
              Value *label = builder.CreateLoad(label_allocations[labelName]);

              Type *set_types[] = {Type::getInt16Ty(context),
                                   Type::getInt8PtrTy(context),
                                   Type::getInt64Ty(context)};
              Value *set_int_arg =
                  llvm::ConstantInt::get(context, llvm::APInt(64, 4, true));
              Value *set_args[] = {label, cast, set_int_arg};
              Type *set_ret_type = Type::getVoidTy(context);
              FunctionCallee set_c = utils::getFunctionCallee(
                  F.getParent(), set_types, set_args, 3, set_ret_type, "dfsan_add_label");
              builder.CreateCall(set_c, set_args);

              errs() << "Added label for variable " << labelName << " on line " << new_line << '\n';
            }
          }
          old_line = new_line;
        }
      }
      return true;
    }
  };
} // namespace

char SetLabels::ID = 0;
static RegisterPass<SetLabels> X("set_labels", "set dfsan labels");

// static llvm::RegisterStandardPasses X(
//     llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
//     [](const llvm::PassManagerBuilder &Builder,
//        llvm::legacy::PassManagerBase &PM) { PM.add(new SetLabels()); });

namespace
{
  struct Instrumenter
  {
    IRBuilder<> builder;
    Module &M;

    Instrumenter(Module &_M) : builder(_M.getContext()),
                               M(_M) {}

    void callExitFunction()
    {

      if (M.getFunction("main"))
      {
        builder.SetInsertPoint(M.getFunction("main")->getEntryBlock().getFirstNonPHI());
        Type *types[] = {builder.getInt64Ty()};
        Value *int_arg = builder.getInt64(64);
        Value *args[] = {int_arg};
        Type *ret_type = builder.getVoidTy();
        utils::getFunctionCallee(&M, types, args, 1, ret_type,
                                 "__dfsw_exit_function_2");
        Function *f = M.getFunction("__dfsw_exit_function_2");

        GlobalValue::LinkageTypes linkage = GlobalValue::ExternalLinkage;
        FunctionType *exit_type = llvm::FunctionType::get(
            builder.getInt32Ty(), {builder.getInt8PtrTy()}, false);

        Function *exit_f =
            llvm::Function::Create(exit_type, linkage, "atexit", &M);

        Value *cast = builder.CreatePointerCast(f, builder.getInt8PtrTy());
        builder.CreateCall(exit_f, {cast});
      }
    }

    void markFunction(Instruction &I)
    {
      int lineNumber = -1;
      if (DILocation *Loc = I.getDebugLoc())
      {
        lineNumber = Loc->getLine();
      }
      FunctionTainter::markFunction(I.getModule()->getSourceFileName(), I.getFunction()->getName().str(), lineNumber);
    }

    void callReadFunctionArgs(Instruction &I)
    {
      builder.SetInsertPoint(I.getNextNode());
      int lineNumber = -1;
      if (DILocation *Loc = I.getDebugLoc())
      {
        lineNumber = Loc->getLine();
      }
      Value *instValue = &(cast<Value>(I));
      if (instValue->getType()->isIntOrIntVectorTy())
      {
        Value *v = builder.CreateZExtOrTrunc(instValue, builder.getInt64Ty());
        Type *types[] = {
            builder.getInt64Ty(),
        };
        Value *args[] = {v};
        Type *ret_type = builder.getInt16Ty();
        FunctionCallee c = utils::getFunctionCallee(&M, types, args, 1, ret_type,
                                                    "dfsan_get_label");
        auto *label = builder.CreateCall(c, args);

        Type *types_print[] = {
            builder.getInt16Ty(), builder.getInt8PtrTy(),
            builder.getInt8PtrTy(), builder.getInt64Ty()};
        Value *fileNameValue = builder.CreateGlobalStringPtr(M.getSourceFileName());
        Value *functionNameValue = builder.CreateGlobalStringPtr(I.getFunction()->getName());
        Value *int_arg_print = builder.getInt64(lineNumber);

        Value *args_print[] = {label, fileNameValue, functionNameValue,
                               int_arg_print};
        Type *ret_type_print = builder.getVoidTy();
        FunctionCallee c_print =
            utils::getFunctionCallee(&M, types_print, args_print, 4,
                                     ret_type_print, "__dfsw_read_function_args");
        builder.CreateCall(c_print, args_print);
      }
    }

    int callReadFunctionArg2(Instruction *inst, int lineNumber, std::string fileName, std::string functionName)
    {
      if (PHINode *phi = dyn_cast<PHINode>(inst))
      {
        return -1;
      }

      if (inst->isTerminator())
      {
        builder.SetInsertPoint(inst);
      }
      else
      {
        builder.SetInsertPoint(inst->getNextNode());
      }
      // for (Use &U : inst->operands())
      // {
      for (llvm::AShrOperator::value_op_iterator U = inst->value_op_begin(), E = inst->value_op_end(); U != E; ++U)
      {
        // j++;
        // if(j == 1){
        //   continue;
        // }
        Value *v;
        // errs() << *v << '\n';

        if (U->getType()->isIntOrIntVectorTy())
        {
          v = builder.CreateZExtOrTrunc(*U, builder.getInt64Ty());
        }
        else
        {
          //errs() << "cannont read label of value " << *v << '\n';
          continue;
        }

        Type *types[] = {
            // Type::getInt16Ty(context),
            // Type::getInt8PtrTy(context),
            builder.getInt64Ty()};
        Value *args[] = {v};
        Type *ret_type = builder.getInt16Ty();
        FunctionCallee c = utils::getFunctionCallee(&M, types, args, 1, ret_type,
                                                    "dfsan_get_label");
        auto *label = builder.CreateCall(c, args);

        Type *types_print[] = {
            builder.getInt16Ty(), builder.getInt8PtrTy(),
            builder.getInt8PtrTy(), builder.getInt64Ty()};

        Value *fileNameValue = builder.CreateGlobalStringPtr(fileName);
        Value *functionNameValue = builder.CreateGlobalStringPtr(functionName);
        Value *int_arg_print = builder.getInt64(lineNumber);
        Value *args_print[] = {label, fileNameValue, functionNameValue,
                               int_arg_print};
        Type *ret_type_print = builder.getVoidTy();
        FunctionCallee c_print = utils::getFunctionCallee(
            &M, types_print, args_print, 4, ret_type_print,
            "__dfsw_read_function_args_2");
        builder.CreateCall(c_print, args_print);
      }
      return 0;
    }

    void checkArgs(CallInst &I)
    {
      for (auto arg = I.arg_begin(); arg != I.arg_end(); ++arg)
      {
        if (arg->get()->getType()->isPointerTy())
        {
          markFunction(I);
        }
      }
    }

    void checkReturn(Instruction &I)
    {
      if (I.getType()->isPointerTy())
      {
        markFunction(I);
      }
      else if (!I.getType()->isVoidTy())
      {
        callReadFunctionArgs(I);
      }
    }
  };

  struct FunctionCallVisitor : public InstVisitor<FunctionCallVisitor>
  {
    Instrumenter *inst;

    FunctionCallVisitor(Instrumenter *_inst) : inst(_inst) {}

    void visitCallInst(CallInst &I)
    {
      Function *calledFunction = I.getCalledFunction();
      if (calledFunction != NULL)
      {
        if ((calledFunction->getName().str()).find("llvm.dbg") == std::string::npos) //look at arg if not debug function
        {
          inst->checkArgs(I);
          inst->checkReturn(I);
        }
      }
      else
      {
        inst->checkArgs(I);
        inst->checkReturn(I);
      }
    }
  };

  struct LoadVisitor : public InstVisitor<LoadVisitor>
  {
    Instrumenter *inst;

    LoadVisitor(Instrumenter *_inst) : inst(_inst) {}

    void visitLoadInst(LoadInst &I)
    {
      errs() << "Load inst" << '\n';
      inst->checkReturn(I);
    }
  };

  struct TaintPass : public ModulePass
  {

    static char ID; // Pass identification, replacement for typeid
    TaintPass() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
      // Read labels
      errs() << "Step 2: Read labels of function arguments" << '\n';

      FunctionTainter::openJson();
      Instrumenter inst(M);

      FunctionCallVisitor FCV(&inst);
      FCV.visit(M);

      LoadVisitor LV(&inst);
      LV.visit(M);

      for (auto curFref = M.getFunctionList().begin(),
                endFref = M.getFunctionList().end();
           curFref != endFref; ++curFref)
      {
        StringRef fNameRef = curFref->getName();
        std::string fName = fNameRef.str();
        Function *F = M.getFunction(fNameRef);
        if (F->getMetadata("dbg"))
        {
          readLabels(*F);
        }
      }
      FunctionTainter::closeJson();
      return true;
    }

    bool readLabels(Function &F)
    {

      errs() << "Read function arguments in function " << F.getName() << '\n';

      LLVMContext &context = F.getContext();
      IRBuilder<> builder(context);

      Instruction *insertPoint =
          utils::findLocation(&(F.getEntryBlock().front()), 0);
      builder.SetInsertPoint(insertPoint);

      //Iterate all statements to analyze function calls

      if (F.getName().str() == "main")
      {
        Type *types[] = {builder.getInt64Ty()};
        Value *int_arg =
            llvm::ConstantInt::get(context, llvm::APInt(64, 0, true));
        Value *args[] = {int_arg};
        Type *ret_type = Type::getVoidTy(context);
        utils::getFunctionCallee(F.getParent(), types, args, 1, ret_type,
                                 "__dfsw_exit_function");
        Function *f = F.getParent()->getFunction("__dfsw_exit_function");

        GlobalValue::LinkageTypes linkage = GlobalValue::ExternalLinkage;
        FunctionType *exit_type = llvm::FunctionType::get(
            builder.getInt32Ty(), {builder.getInt8PtrTy()}, false);

        Function *exit_f =
            llvm::Function::Create(exit_type, linkage, "atexit", F.getParent());

        Value *cast = builder.CreatePointerCast(f, builder.getInt8PtrTy());
        builder.CreateCall(exit_f, {cast});
      }

      // errs() << "inert " << *insertPoint << '\n';
      std::string fileName = F.getParent()->getSourceFileName();
      MDNode *meta = F.getMetadata("dbg");
      int lineNumber = -1;
      if (DISubprogram *SP = dyn_cast<DISubprogram>(meta))
      {
        lineNumber = SP->getLine();
      }
      for (auto arg = F.arg_begin(); arg != F.arg_end(); ++arg)
      {

        std::string argName = arg->getName().str();

        Value *v;
        if (arg->getType()->isPointerTy()) //If the argument is a pointer, the whole function has to be inspected
        {
          FunctionTainter::markFunction(F.getParent()->getSourceFileName(), F.getName().str(), lineNumber);
        }
        else if (arg->getType()->isIntOrIntVectorTy()) //If the argument is a primitive type, we only have to inspect the function if the argument is tainted
        {
          errs() << arg->getType()->isIntOrIntVectorTy() << '\n';
          v = builder.CreateZExtOrTrunc(&*arg, builder.getInt64Ty());
          Type *types[] = {
              Type::getInt64Ty(context),
          };
          Value *args[] = {v};
          Type *ret_type = Type::getInt16Ty(context);
          FunctionCallee c = utils::getFunctionCallee(F.getParent(), types, args, 1, ret_type,
                                                      "dfsan_get_label");
          auto *label = builder.CreateCall(c, args);

          Type *types_print[] = {
              Type::getInt16Ty(context), Type::getInt8PtrTy(context),
              Type::getInt8PtrTy(context), Type::getInt64Ty(context)};
          Value *fileNameValue = builder.CreateGlobalStringPtr(fileName);
          Value *functionNameValue = builder.CreateGlobalStringPtr(F.getName());
          Value *int_arg_print =
              llvm::ConstantInt::get(context, llvm::APInt(64, lineNumber, true));
          Value *args_print[] = {label, fileNameValue, functionNameValue,
                                 int_arg_print};
          Type *ret_type_print = Type::getVoidTy(context);
          FunctionCallee c_print =
              utils::getFunctionCallee(F.getParent(), types_print, args_print, 4,
                                       ret_type_print, "__dfsw_read_function_args");
          builder.CreateCall(c_print, args_print);
        }
      }

      return true;
    }
  };
} // namespace

char TaintPass::ID = 1;
static RegisterPass<TaintPass> Y("taint", "Taint analysis pass");

// static llvm::RegisterStandardPasses Y(
//     llvm::PassManagerBuilder::EP_ModuleOptimizerEarly,
//     [](const llvm::PassManagerBuilder &Builder,
//        llvm::legacy::PassManagerBase &PM) { PM.add(new TaintPass()); });

namespace
{
  // Hello - The first implementation, without getAnalysisUsage.
  struct TaintFunction : public ModulePass
  {

    static char ID; // Pass identification, replacement for typeid
    TaintFunction() : ModulePass(ID) {}

    bool runOnModule(Module &M) override
    {
      errs() << "Step 3: inspect functions that were changed or have tainted arguments" << '\n';
      std::ifstream changes("tainted_functions.json");
      json jf = json::parse(changes);
      std::string file = M.getSourceFileName();
      Instrumenter inst(M);
      inst.callExitFunction();

      if (jf.contains(file))
      {
        errs() << file << '\n';
        for (auto curFref = M.getFunctionList().begin(),
                  endFref = M.getFunctionList().end();
             curFref != endFref; ++curFref)
        {
          StringRef fNameRef = curFref->getName();
          Function *F = M.getFunction(fNameRef);
          Metadata *meta = F->getMetadata("dbg");
          if (!meta)
          {
            continue;
          }
          if (DISubprogram *SP = dyn_cast<DISubprogram>(meta))
          {
            std::string name = F->getName().str();
            //errs() << "subprogram " << name << '\n';
            if (jf[file].contains(name))
            {
              errs() << "Inspect labels " << file << " in function " << name
                     << '\n';
              readLabels(*F, jf[file][name]["variables"], inst);
            }
          }
        }
      }

      std::ifstream changes_2("changes.json");
      json jf_2 = json::parse(changes_2);
      if (jf_2.contains(file))
      {
        for (auto curFref = M.getFunctionList().begin(),
                  endFref = M.getFunctionList().end();
             curFref != endFref; ++curFref)
        {
          StringRef fNameRef = curFref->getName();
          Function *F = M.getFunction(fNameRef);
          Metadata *meta = F->getMetadata("dbg");

          if (!meta)
          {
            continue;
          }
          if (DISubprogram *SP = dyn_cast<DISubprogram>(meta))
          {
            std::string name = SP->getName().str();
            if (jf_2[file].contains(name))
            {
              if (jf_2[file][name]["line"] == SP->getLine())
              {
                errs() << "Inspect labels in " << file << " in function " << name
                       << '\n';
                readLabels(*F, jf_2[file][name]["variables"], inst);
              }
            }
          }
        }
      }
      return true;
    }

    bool readLabels(Function &F, json changes, Instrumenter instr)
    {
      std::string fileName = F.getParent()->getSourceFileName();
      LLVMContext &context = F.getContext();
      IRBuilder<> builder(context);
      Instruction *insertPoint =
          utils::findLocation(&(F.getEntryBlock().front()), 0);
      builder.SetInsertPoint(insertPoint);
      errs() << F.getName().str() << '\n';
      // if (F.getName().str() == "main")
      // {
      //   errs()<<"insert exit"<<'\n';
      //   Type *types[] = {builder.getInt64Ty()};
      //   Value *int_arg =
      //       llvm::ConstantInt::get(context, llvm::APInt(64, 0, true));
      //   Value *args[] = {int_arg};
      //   Type *ret_type = Type::getVoidTy(context);
      //   utils::getFunctionCallee(F.getParent(), types, args, 1, ret_type,
      //                            "__dfsw_exit_function_2");
      //   Function *f = F.getParent()->getFunction("__dfsw_exit_function_2");

      //   GlobalValue::LinkageTypes linkage = GlobalValue::ExternalLinkage;
      //   FunctionType *exit_type = llvm::FunctionType::get(
      //       builder.getInt32Ty(), {builder.getInt8PtrTy()}, false);

      //   Function *exit_f =
      //       llvm::Function::Create(exit_type, linkage, "atexit", F.getParent());

      //   Value *cast = builder.CreatePointerCast(f, builder.getInt8PtrTy());
      //   builder.CreateCall(exit_f, {cast});
      // }

      std::vector<Instruction *> worklist;
      std::vector<int> lines;

      for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
      {
        if (DILocation *Loc = I->getDebugLoc())
        {
          unsigned line = Loc->getLine();
          worklist.push_back(&*I);
          lines.push_back(line);
        }
      }

      int len = worklist.size();

      for (int i = 0; i < len; i++)
      {
        Instruction *inst = worklist[i];
        // if (!inst->getNextNode())
        // {
        //   continue;
        // }
        //errs() << *inst << '\n';
        // if(inst->isTerminator()){
        //   //builder.SetInsertPoint(inst->getParent()->getTerminator()->getPrevNode());
        //   errs() << "Terminator" << '\n';
        //   continue;
        // } else{
        // }
        //errs() << *inst << '\n';

        int x = instr.callReadFunctionArg2(inst, lines[i], fileName, F.getName());

        // if (PHINode *phi = dyn_cast<PHINode>(inst))
        // {
        //   continue;
        // }

        // if (inst->isTerminator())
        // {
        //   builder.SetInsertPoint(inst);
        // }
        // else
        // {
        //   builder.SetInsertPoint(inst->getNextNode());
        // }

        // int lineNumber = lines[i];
        // // for (Use &U : inst->operands())
        // // {
        // for (llvm::AShrOperator::value_op_iterator U = inst->value_op_begin(), E = inst->value_op_end(); U != E; ++U)
        // {
        //   // j++;
        //   // if(j == 1){
        //   //   continue;
        //   // }
        //   Value *v;
        //   // errs() << *v << '\n';

        //   if (U->getType()->isIntOrIntVectorTy())
        //   {
        //     v = builder.CreateZExtOrTrunc(*U, builder.getInt64Ty());
        //   }
        //   else
        //   {
        //     //errs() << "cannont read label of value " << *v << '\n';
        //     continue;
        //   }

        //   Type *types[] = {
        //       // Type::getInt16Ty(context),
        //       // Type::getInt8PtrTy(context),
        //       Type::getInt64Ty(context),
        //   };
        //   Value *args[] = {v};
        //   Type *ret_type = Type::getInt16Ty(context);
        //   FunctionCallee c = utils::getFunctionCallee(F.getParent(), types, args, 1, ret_type,
        //                                               "dfsan_get_label");
        //   auto *label = builder.CreateCall(c, args);

        //   Type *types_print[] = {
        //       Type::getInt16Ty(context), Type::getInt8PtrTy(context),
        //       Type::getInt8PtrTy(context), Type::getInt64Ty(context)};

        //   Value *fileNameValue = builder.CreateGlobalStringPtr(fileName);
        //   Value *functionNameValue = builder.CreateGlobalStringPtr(F.getName());
        //   Value *int_arg_print =
        //       llvm::ConstantInt::get(context, llvm::APInt(64, lineNumber, true));
        //   Value *args_print[] = {label, fileNameValue, functionNameValue,
        //                          int_arg_print};
        //   Type *ret_type_print = Type::getVoidTy(context);
        //   FunctionCallee c_print = utils::getFunctionCallee(
        //       F.getParent(), types_print, args_print, 4, ret_type_print,
        //       "__dfsw_read_function_args_2");
        //   builder.CreateCall(c_print, args_print);
        // }
      }

      // for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b)
      // {

      //   errs() << *b << '\n';
      // }

      return true;
    }
  };
} // namespace

char TaintFunction::ID = 2;
static RegisterPass<TaintFunction> Z("taint_function",
                                     "second iteration of taint analysis pass");
