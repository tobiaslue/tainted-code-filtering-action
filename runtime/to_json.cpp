#include <iostream>
#include <fstream>
#include <sanitizer/dfsan_interface.h>
#include "taint.h"
#include </usr/local/json/single_include/nlohmann/json.hpp>
//#include <sqlite3.h>
using json = nlohmann::json;

std::ifstream tf("tainted_functions.json");
json j = json::parse(tf);
json j_2;
//sqlite3 *db;
char *zErrMsg = 0;
//int rc = sqlite3_open("dynamic_blame.db", &db);
using Record = std::vector<std::string>;
using Records = std::vector<Record>;

std::ifstream changes("changes.json");
json jf = json::parse(changes);

void __dfsw_read_function_args(dfsan_label label, char *fileName, char *functionName, int line)
{
	const struct dfsan_label_info *info = dfsan_get_label_info(label);
	if (info->desc)
	{
		j[fileName][functionName]["line"] = line;
	}
	if (info->l1)
	{
		j[fileName][functionName]["line"] = line;
	}
	if (info->l2)
	{
		j[fileName][functionName]["line"] = line;
	}
}

void __dfsw_exit_function(int x)
{
	std::string fileName = "tainted_functions.json";
	std::ofstream file(fileName, std::ofstream::out);
	file << j.dump(2) << std::endl;
	file.close();
}

// static int callback(void *data, int argc, char **argv, char **azColName)
// {
// 	Records *records = static_cast<Records *>(data);
// 	records->emplace_back(argv, argv + argc);
// 	return 0;
// }

// int getCommitByLabel(const char *label)
// {
// 	for (auto &file : jf.items())
// 	{
// 		for (auto &fun : file.value().items())
// 		{
// 			for (auto &v : fun.value()["variables"])
// 			{
// 				if (v["label"] == label)
// 				{
// 					return v["commit_id"];
// 				}
// 			}
// 		}
// 	}
// }

// void write_to_db(const char *label, char *fileName, char *functionName, int line)
// {
// 	std::string sql = "INSERT OR IGNORE INTO files (name) VALUES ('";
// 	sql += fileName;
// 	sql += "')";
// 	rc = sqlite3_exec(db, &sql[0], NULL, NULL, &zErrMsg);
// 	Records records;
// 	//std::cout<<functionName<<'\n';
// 	sql = "SELECT file_id from files WHERE name='";
// 	sql += fileName;
// 	sql += "'";

// 	rc = sqlite3_exec(db, &sql[0], callback, &records, &zErrMsg);

// 	if (rc)
// 	{
// 		std::cout << zErrMsg << '\n';
// 	}

// 	std::string file_id = records[0][0];
// 	sql = "INSERT OR IGNORE INTO functions (file_id, name) VALUES ('";
// 	sql += file_id;
// 	sql += "', '";
// 	sql += functionName;
// 	sql += "')";
// 	rc = sqlite3_exec(db, &sql[0], NULL, NULL, &zErrMsg);
// 	if (rc)
// 	{
// 		std::cout << zErrMsg << '\n';
// 	}
// 	// Records commit_records;
// 	// sql = "SELECT commit_id FROM commits WHERE hash='";
// 	// sql += commit;
// 	// sql += "'";
// 	// std::cout<<sql<<'\n';
// 	// rc = sqlite3_exec(db, &sql[0], callback, &commit_records, &zErrMsg);
// 	// std::cout<<"rc "<<rc<<'\n';
// 	// if(rc){
// 	// 	std::cout<<zErrMsg<<'\n';
// 	// }
// 	// std::cout<<"no error"<<'\n';
// 	// std::string commit_id = commit_records[0][0];
// 	// std::cout<<"still no error"<<'\n';
// 	// std::cout<< "commit_id " << commit_id << '\n';

// 	Records function_records;
// 	sql = "SELECT function_id from functions WHERE file_id='";
// 	sql += file_id;
// 	sql += "' AND name='";
// 	sql += functionName;
// 	sql += "'";
// 	//std::cout<<sql<<'\n';
// 	rc = sqlite3_exec(db, &sql[0], callback, &function_records, &zErrMsg);
// 	if (rc)
// 	{
// 		std::cout << zErrMsg << '\n';
// 	}
// 	std::string function_id = function_records[0][0];

// 	std::string commit = std::to_string(getCommitByLabel(label));

// 	sql = "INSERT OR IGNORE INTO changes (commit_id, function_id, line, label) VALUES ('";
// 	sql += commit;
// 	sql += "', '";
// 	sql += function_id;
// 	sql += "', '";
// 	sql += std::to_string(line);
// 	sql += "', '";
// 	sql += label;
// 	sql += "')";
// 	rc = sqlite3_exec(db, &sql[0], NULL, NULL, &zErrMsg);

// 	if (rc)
// 	{
// 		std::cout << zErrMsg << '\n';
// 		std::cout << commit << " " << function_id << " " << std::to_string(line) << " " << label << '\n';
// 	}
// }

void __dfsw_read_function_args_2(dfsan_label label, char *fileName, char *functionName, int line)
{
	const struct dfsan_label_info *info = dfsan_get_label_info(label);

	if (info->desc)
	{
		std::string lineStr = std::to_string(line);
		if (!j_2[fileName][functionName].contains(lineStr))
		{
			j_2[fileName][functionName][lineStr]["desc"] = {};
		}
		j_2[fileName][functionName][lineStr]["desc"].push_back(info->desc);
		//write_to_db(info->desc, fileName, functionName, line);
	}
	else
	{
		if (info->l1)
		{
			__dfsw_read_function_args_2(info->l1, fileName, functionName, line);
		}
		if (info->l2)
		{
			__dfsw_read_function_args_2(info->l2, fileName, functionName, line);
		}
	}
}

void __dfsw_exit_function_2(int x)
{
	std::string fileName = "tainted_lines.json";
	std::ofstream file(fileName, std::ofstream::out);
	std::cout << "save file" << '\n';
	file << j_2.dump(2) << std::endl;
	file.close();
	//sqlite3_close(db);
}