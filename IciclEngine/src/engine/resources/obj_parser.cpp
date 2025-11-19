#include <engine/resources/obj_parser.h>
#include <fstream>
#include <sstream>
#include <engine/utilities/macros.h>
#include <algorithm>
#include <chrono>
#include <engine/utilities/utilities.h>

ObjMesh ObjParser::load_mesh_from_filepath(const std::string& a_path)
{
    HighResolutionTimer timer;
    timer.start();
    std::vector<ObjVertex> vertices;
    std::ifstream file(a_path);
    std::string line;

    ObjMesh obj_mesh;

    if (!file.is_open())
    {
        PRINTLN("failed open file at: {}", a_path);
        return obj_mesh;
    }
    
    while (std::getline(file, line))
    {
        std::istringstream string_stream(line);
        std::string prefix;
        string_stream >> prefix;
        if (prefix == "v")
        {
            ObjPosition obj_pos;
            ObjColor obj_clr;
            string_stream >> obj_pos.x >> obj_pos.y >> obj_pos.z >> obj_clr.r >> obj_clr.g >> obj_clr.b;
            obj_pos.vec3 = glm::vec3(obj_pos.x, obj_pos.y, obj_pos.z);
            obj_clr.r = std::max(0.0f, std::min(1.0f, obj_clr.r));
            obj_clr.g = std::max(0.0f, std::min(1.0f, obj_clr.g));
            obj_clr.b = std::max(0.0f, std::min(1.0f, obj_clr.b));
            obj_clr.vec3 = glm::vec3(obj_clr.r ,obj_clr.g ,obj_clr.b);
            obj_mesh.verticies.push_back(obj_pos);
            obj_mesh.colors.push_back(obj_clr);
        }
        else if (prefix == "vn")
        {
            ObjNormal obj_nrm;
            string_stream >> obj_nrm.x >> obj_nrm.y >> obj_nrm.z;
            obj_nrm.x = std::max(0.0f, std::min(1.0f, obj_nrm.x));
            obj_nrm.y = std::max(0.0f, std::min(1.0f, obj_nrm.y));
            obj_nrm.z = std::max(0.0f, std::min(1.0f, obj_nrm.z));
            obj_nrm.vec3 = glm::vec3(obj_nrm.x, obj_nrm.y, obj_nrm.z);
            obj_mesh.normals.push_back(obj_nrm);
        }
        else if (prefix == "vt")
        {
            ObjUVs obj_uv;
            string_stream >> obj_uv.x >> obj_uv.y >> obj_uv.z;
            obj_uv.x = std::max(0.0f, std::min(1.0f, obj_uv.x));
            obj_uv.y = std::max(0.0f, std::min(1.0f, obj_uv.y));
            obj_uv.z = std::max(0.0f, std::min(1.0f, obj_uv.z));
            obj_uv.vec3 = glm::vec3(obj_uv.x, obj_uv.y, obj_uv.z);
            obj_mesh.uvs.push_back(obj_uv);
        }
        else if (prefix == "f")
        {
            
            ObjFace obj_face;
            line.erase(0, 1);

            std::istringstream faces_string_stream(line);
            std::string face_string;

            size_t index = 0;
            while (faces_string_stream >> face_string)
            {
                ObjFaceVertex obj_face_vert;
                std::replace(face_string.begin(), face_string.end(), '/', ' ');
                std::istringstream face_stream(face_string);

                glm::uint* spot_2 = &obj_face_vert.nrm;
                if (!(obj_mesh.normals.size() > 0)) spot_2 = &obj_face_vert.uv;

                face_stream >> obj_face_vert.pos >> *spot_2 >> obj_face_vert.uv;
                obj_face_vert.pos = std::max(glm::uint(0), std::min( glm::uint(obj_mesh.verticies.size()), obj_face_vert.pos));
                obj_face_vert.nrm = std::max(glm::uint(0), std::min(glm::uint(obj_mesh.normals.size()), obj_face_vert.nrm));
                obj_face_vert.uv = std::max(glm::uint(0), std::min(glm::uint(obj_mesh.uvs.size()), obj_face_vert.uv));

                obj_face_vert.vec3 = glm::uvec3(obj_face_vert.pos, obj_face_vert.nrm, obj_face_vert.uv);
                obj_face.indicies.push_back(obj_face_vert);
            }
            obj_mesh.faces.push_back(obj_face);
        }
    }
    obj_mesh.path = a_path;
    timer.stop();
    PRINTLN("time to load {}: {}ms", a_path, timer.get_time_ms());
    return ObjMesh();
}
