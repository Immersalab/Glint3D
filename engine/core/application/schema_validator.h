#pragma once

#include <string>
#include <memory>

class SchemaValidator {
public:
    enum class ValidationResult {
        Success,
        SchemaLoadError,
        ValidationError,
        ParseError
    };
    
    struct ValidationResponse {
        ValidationResult result = ValidationResult::Success;
        std::string errorMessage;
        std::string detailedErrors;
    };
    
    SchemaValidator();
    ~SchemaValidator();
    
    // Load schema from file
    bool loadSchema(const std::string& schemaPath);
    
    // Load schema from string content  
    bool loadSchemaFromString(const std::string& schemaContent);
    
    // Validate JSON against the loaded schema
    ValidationResponse validate(const std::string& jsonContent);
    
    // Static method to get embedded schema for v1.3
    static std::string getEmbeddedSchemaV1_3();
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};