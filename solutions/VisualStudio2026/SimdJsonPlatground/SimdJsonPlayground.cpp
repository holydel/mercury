#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "simdjson.h"

void StartObjectWithKey(simdjson::builder::string_builder& builder, const std::string& key) {
    builder.append_raw(key);
    builder.append_colon();
    builder.start_object();
}

void StartArrayWithKey(simdjson::builder::string_builder& builder, const std::string& key) {
    builder.append_raw(key);
    builder.append_colon();
    builder.start_array();
}

// Base class for assets
class Asset {
public:
    virtual ~Asset() = default;
    std::string type  = "none";
    std::string path = "";
    std::string date = "";
    
    // Virtual method for serialization
    virtual void serialize_to(simdjson::builder::string_builder& builder) const {
        builder.start_object();
        builder.append_key_value("type", type);
        builder.append_key_value("path", path);
        builder.append_key_value("date", date);
    }
};

// File asset
class FileAsset : public Asset {
public:
    FileAsset() { type = "file"; }
    int64_t size;
    
    void serialize_to(simdjson::builder::string_builder& builder) const override {
        Asset::serialize_to(builder);
        builder.append_key_value("size", size);
        builder.end_object();
    }
};

// Folder asset
class FolderAsset : public Asset {
public:
    FolderAsset() { type = "folder"; }
    std::vector<std::unique_ptr<Asset>> content;
    
    void serialize_to(simdjson::builder::string_builder& builder) const override {
        Asset::serialize_to(builder);
        
        // Add content array
        StartObjectWithKey(builder,"content");
        builder.start_array();
        for (const auto& asset : content) {
            asset->serialize_to(builder);
        }
        builder.end_array();
        
        builder.end_object();
    }
};

// Project class
class Project {
public:
    std::string name = "noname";
    std::string version = "0.1.0";
    std::vector<std::unique_ptr<Asset>> assets;
    
    void serialize_to(simdjson::builder::string_builder& builder) const {
        builder.start_object();
        StartObjectWithKey(builder, "project");
		builder.append_key_value("name", name); builder.append_comma();
        builder.append_key_value("version", version); builder.append_comma();
        
		StartArrayWithKey(builder, "assets");
        builder.end_array();
        // Add assets array
        //StartObjectWithKey(builder, "assets");
        //builder.start_array();
        //for (const auto& asset : assets) {
        //    asset->serialize_to(builder);
        //}
        //builder.end_array();
        
        builder.end_object();
        builder.end_object();
    }
};

int main()
{
    // Create project structure
    Project project;
    project.name = "TestBed Mercury Project";
    project.version = "0.0.1";
    
    //// Create main folder
    //auto mainFolder = std::make_unique<FolderAsset>();
    //mainFolder->path = "C:/Data";
    //mainFolder->date = "2024-06-01T1200: 00Z";
    //
    //// Add files to main folder
    //auto file1 = std::make_unique<FileAsset>();
    //file1->path = "C:/Data/file1.txt";
    //file1->size = 1024;
    //file1->date = "2024-06-01T1200: 00Z";
    //mainFolder->content.push_back(std::move(file1));
    //
    //auto file2 = std::make_unique<FileAsset>();
    //file2->path = "C:/Data/file2.txt";
    //file2->size = 2048;
    //file2->date = "2024-06-01T1200: 00Z";
    //mainFolder->content.push_back(std::move(file2));
    //
    //// Create subfolder
    //auto subFolder = std::make_unique<FolderAsset>();
    //subFolder->path = "C:/Data/SubFolder";
    //subFolder->date = "2024-06-01T1200: 00Z";
    //
    //// Add file to subfolder
    //auto file3 = std::make_unique<FileAsset>();
    //file3->path = "C:/Data/SubFolder/file3.txt";
    //file3->size = 512;
    //file3->date = "2024-06-01T1200: 00Z";
    //subFolder->content.push_back(std::move(file3));
    //
    //// Add subfolder to main folder
    //mainFolder->content.push_back(std::move(subFolder));
    //
    //// Add main folder to project
    //project.assets.push_back(std::move(mainFolder));
    
    // Serialize to JSON
    simdjson::builder::string_builder sb;
    project.serialize_to(sb);
    auto sv = sb.view().value_unsafe();
    
    std::cout << sv << std::endl;
    std::cin.get();
}