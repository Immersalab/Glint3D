#include "schema_validator.h"
#include <rapidjson/document.h>
#include <rapidjson/schema.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>
#include <sstream>

class SchemaValidator::Impl {
public:
    std::unique_ptr<rapidjson::SchemaDocument> schema;
    std::unique_ptr<rapidjson::SchemaValidator> validator;
    
    bool loadSchema(const std::string& schemaContent) {
        using namespace rapidjson;
        
        Document schemaDoc;
        if (schemaDoc.Parse(schemaContent.c_str()).HasParseError()) {
            return false;
        }
        
        try {
            schema = std::make_unique<SchemaDocument>(schemaDoc);
            validator = std::make_unique<rapidjson::SchemaValidator>(*schema);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    SchemaValidator::ValidationResponse validateDocument(const rapidjson::Document& document) {
        using namespace rapidjson;
        SchemaValidator::ValidationResponse response;
        
        if (!schema || !validator) {
            response.result = SchemaValidator::ValidationResult::SchemaLoadError;
            response.errorMessage = "No schema loaded";
            return response;
        }
        
        // Reset validator for new validation
        validator->Reset();
        
        if (!document.Accept(*validator)) {
            response.result = SchemaValidator::ValidationResult::ValidationError;
            
            // Get validation errors
            StringBuffer sb;
            validator->GetInvalidSchemaPointer().StringifyUriFragment(sb);
            std::string schemaPath = sb.GetString();
            
            sb.Clear();
            validator->GetInvalidDocumentPointer().StringifyUriFragment(sb);
            std::string documentPath = sb.GetString();
            
            response.errorMessage = "Validation failed";
            response.detailedErrors = "Schema violation at " + documentPath + 
                                    " (schema path: " + schemaPath + "): " + 
                                    validator->GetInvalidSchemaKeyword();
            
            return response;
        }
        
        response.result = SchemaValidator::ValidationResult::Success;
        return response;
    }
};

SchemaValidator::SchemaValidator() : m_impl(std::make_unique<Impl>()) {
}

SchemaValidator::~SchemaValidator() = default;

bool SchemaValidator::loadSchema(const std::string& schemaPath) {
    std::ifstream file(schemaPath);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return m_impl->loadSchema(buffer.str());
}

bool SchemaValidator::loadSchemaFromString(const std::string& schemaContent) {
    return m_impl->loadSchema(schemaContent);
}

SchemaValidator::ValidationResponse SchemaValidator::validate(const std::string& jsonContent) {
    ValidationResponse response;
    
    if (!m_impl->schema || !m_impl->validator) {
        response.result = ValidationResult::SchemaLoadError;
        response.errorMessage = "No schema loaded";
        return response;
    }
    
    using namespace rapidjson;
    
    Document document;
    if (document.Parse(jsonContent.c_str()).HasParseError()) {
        response.result = ValidationResult::ParseError;
        response.errorMessage = "JSON parse error: " + std::string(GetParseError_En(document.GetParseError()));
        return response;
    }
    
    return m_impl->validateDocument(document);
}

std::string SchemaValidator::getEmbeddedSchemaV1_3() {
    return R"({
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Glint JSON Ops v1.3",
  "type": ["object", "array"],
  "definitions": {
    "vec3": {
      "type": "array",
      "items": { "type": "number" },
      "minItems": 3,
      "maxItems": 3
    },
    "opLoad": {
      "type": "object",
      "required": ["op", "path"],
      "properties": {
        "op": { "const": "load" },
        "path": { "type": "string" },
        "name": { "type": "string" },
        "position": { "$ref": "#/definitions/vec3" },
        "scale": { "$ref": "#/definitions/vec3" },
        "transform": {
          "type": "object",
          "properties": {
            "position": { "$ref": "#/definitions/vec3" },
            "scale": { "$ref": "#/definitions/vec3" }
          },
          "additionalProperties": false
        }
      },
      "additionalProperties": false
    },
    "opSetCamera": {
      "type": "object",
      "required": ["op", "position"],
      "properties": {
        "op": { "const": "set_camera" },
        "position": { "$ref": "#/definitions/vec3" },
        "target": { "$ref": "#/definitions/vec3" },
        "front": { "$ref": "#/definitions/vec3" },
        "up": { "$ref": "#/definitions/vec3" },
        "fov": { "type": "number" },
        "fov_deg": { "type": "number" },
        "near": { "type": "number" },
        "far": { "type": "number" }
      },
      "additionalProperties": false
    },
    "opAddLight": {
      "type": "object",
      "required": ["op"],
      "properties": {
        "op": { "const": "add_light" },
        "type": { 
          "type": "string", 
          "enum": ["point", "directional", "spot"] 
        },
        "position": { "$ref": "#/definitions/vec3" },
        "direction": { "$ref": "#/definitions/vec3" },
        "color": { "$ref": "#/definitions/vec3" },
        "intensity": { "type": "number" }
      },
      "additionalProperties": false,
      "anyOf": [
        {
          "properties": {
            "type": { "const": "point" },
            "position": { "$ref": "#/definitions/vec3" }
          },
          "not": { "required": ["direction"] }
        },
        {
          "properties": {
            "type": { "const": "directional" },
            "direction": { "$ref": "#/definitions/vec3" }
          },
          "not": { "required": ["position"] }
        },
        {
          "properties": {
            "type": { "const": "spot" },
            "position": { "$ref": "#/definitions/vec3" },
            "direction": { "$ref": "#/definitions/vec3" },
            "inner_deg": { "type": "number" },
            "outer_deg": { "type": "number" }
          }
        },
        {
          "not": { "required": ["type"] },
          "required": ["position"]
        }
      ]
    },
    "opSetMaterial": {
      "type": "object",
      "required": ["op", "target", "material"],
      "properties": {
        "op": { "const": "set_material" },
        "target": { "type": "string" },
        "material": {
          "type": "object",
          "properties": {
            "color": { "$ref": "#/definitions/vec3" },
            "roughness": { "type": "number" },
            "metallic": { "type": "number" },
            "specular": { "$ref": "#/definitions/vec3" },
            "ambient": { "$ref": "#/definitions/vec3" }
          },
          "additionalProperties": false
        }
      },
      "additionalProperties": false
    },
    "opTransform": {
      "type": "object",
      "required": ["op", "name"],
      "properties": {
        "op": { "const": "transform" },
        "name": { "type": "string" },
        "translate": { "$ref": "#/definitions/vec3" },
        "rotate": {
          "type": "array",
          "items": { "type": "number" },
          "minItems": 3,
          "maxItems": 3
        },
        "scale": { "$ref": "#/definitions/vec3" },
        "setPosition": { "$ref": "#/definitions/vec3" }
      },
      "additionalProperties": false
    },
    "opDelete": {
      "type": "object",
      "required": ["op", "name"],
      "properties": {
        "op": { 
          "const": "delete",
          "deprecated": true,
          "description": "DEPRECATED: Use 'remove' instead. This operation is an alias for 'remove'."
        },
        "name": { "type": "string" }
      },
      "additionalProperties": false
    },
    "opDuplicate": {
      "type": "object",
      "required": ["op", "source", "name"],
      "properties": {
        "op": { "const": "duplicate" },
        "source": { "type": "string" },
        "name": { "type": "string" },
        "position": { "$ref": "#/definitions/vec3" },
        "scale": { "$ref": "#/definitions/vec3" },
        "rotation": { "$ref": "#/definitions/vec3" }
      },
      "additionalProperties": false
    },
    "opRemove": {
      "type": "object",
      "required": ["op", "name"],
      "properties": {
        "op": { 
          "const": "remove",
          "description": "Remove an object from the scene. This is the canonical operation; 'delete' is deprecated."
        },
        "name": { "type": "string" }
      },
      "additionalProperties": false
    },
    "opSetCameraPreset": {
      "type": "object",
      "required": ["op", "preset"],
      "properties": {
        "op": { "const": "set_camera_preset" },
        "preset": { 
          "type": "string", 
          "enum": ["front", "back", "left", "right", "top", "bottom", "iso_fl", "iso-fl", "iso_br", "iso-br"] 
        },
        "target": { "$ref": "#/definitions/vec3" },
        "fov": { "type": "number", "minimum": 0.1, "maximum": 179.9 },
        "margin": { "type": "number", "minimum": 0 }
      },
      "additionalProperties": false
    },
    "opOrbitCamera": {
      "type": "object",
      "required": ["op"],
      "properties": {
        "op": { "const": "orbit_camera" },
        "yaw": { "type": "number" },
        "pitch": { "type": "number" },
        "center": { "$ref": "#/definitions/vec3" }
      },
      "additionalProperties": false
    },
    "opFrameObject": {
      "type": "object",
      "required": ["op", "name"],
      "properties": {
        "op": { "const": "frame_object" },
        "name": { "type": "string" },
        "margin": { "type": "number", "minimum": 0 }
      },
      "additionalProperties": false
    },
    "opSelect": {
      "type": "object",
      "required": ["op", "name"],
      "properties": {
        "op": { "const": "select" },
        "name": { "type": "string" }
      },
      "additionalProperties": false
    },
    "opSetBackground": {
      "type": "object",
      "required": ["op"],
      "properties": {
        "op": { "const": "set_background" },
        "color": { "$ref": "#/definitions/vec3" },
        "top": { "$ref": "#/definitions/vec3" },
        "bottom": { "$ref": "#/definitions/vec3" },
        "hdr": { "type": "string" },
        "skybox": { "type": "string" }
      },
      "additionalProperties": false,
      "anyOf": [
        { "required": ["color"] },
        { "required": ["top", "bottom"] },
        { "required": ["hdr"] },
        { "required": ["skybox"] }
      ]
    },
    "opExposure": {
      "type": "object",
      "required": ["op", "value"],
      "properties": {
        "op": { "const": "exposure" },
        "value": { "type": "number" }
      },
      "additionalProperties": false
    },
    "opToneMap": {
      "type": "object",
      "required": ["op", "type"],
      "properties": {
        "op": { "const": "tone_map" },
        "type": { 
          "type": "string", 
          "enum": ["linear", "reinhard", "filmic", "aces"] 
        },
        "gamma": { "type": "number", "minimum": 0.1 }
      },
      "additionalProperties": false
    },
    "opRenderImage": {
      "type": "object",
      "required": ["op", "path"],
      "properties": {
        "op": { "const": "render_image" },
        "path": { "type": "string" },
        "width": { "type": "integer", "minimum": 1 },
        "height": { "type": "integer", "minimum": 1 }
      },
      "additionalProperties": false
    },
    "op": {
      "oneOf": [
        { "$ref": "#/definitions/opLoad" },
        { "$ref": "#/definitions/opSetCamera" },
        { "$ref": "#/definitions/opAddLight" },
        { "$ref": "#/definitions/opSetMaterial" },
        { "$ref": "#/definitions/opTransform" },
        { "$ref": "#/definitions/opDelete" },
        { "$ref": "#/definitions/opDuplicate" },
        { "$ref": "#/definitions/opRemove" },
        { "$ref": "#/definitions/opSetCameraPreset" },
        { "$ref": "#/definitions/opOrbitCamera" },
        { "$ref": "#/definitions/opFrameObject" },
        { "$ref": "#/definitions/opSelect" },
        { "$ref": "#/definitions/opSetBackground" },
        { "$ref": "#/definitions/opExposure" },
        { "$ref": "#/definitions/opToneMap" },
        { "$ref": "#/definitions/opRenderImage" }
      ]
    }
  },
  "oneOf": [
    {
      "type": "object",
      "properties": {
        "version": { "type": "integer", "enum": [1] },
        "ops": {
          "type": "array",
          "items": { "$ref": "#/definitions/op" }
        }
      },
      "required": ["ops"],
      "additionalProperties": false
    },
    {
      "type": "array",
      "items": { "$ref": "#/definitions/op" }
    }
  ]
})";
}
