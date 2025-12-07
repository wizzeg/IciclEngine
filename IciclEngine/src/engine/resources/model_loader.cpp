#include <engine/resources/model_loader.h>
#include <map>
#include <engine/utilities/macros.h>
#include <engine/utilities/utilities.h>
#include <engine/utilities/memory_checking.h>
#include <fstream>
#include <sstream>
#include <algorithm>

MeshData ModelLoader::load_obj_mesh_from_file(const std::string a_path)
{
    HighResolutionTimer timer;
    TimeNow time_now;
    std::string title = a_path + " loading started at";
    timer.start();
    time_now.print_time(title);
    std::vector<ObjVertex> vertices;
    std::ifstream file(a_path);
    std::string line;

    MeshData mesh;
    mesh.started_load = true;
    mesh.path_hashed = hashed_string_64(a_path.c_str());
    mesh.ram_load_status = ELoadStatus::StartedLoad;
    if (!file.is_open())
    {
        PRINTLN("failed open file at: {}", a_path);
        mesh.bad_load = true;
        mesh.ram_load_status = ELoadStatus::FailedLoadOpen;
        return mesh;
    }
    file.seekg(0, std::ios::end);
    std::streampos model_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (((float)(model_size / (1024.0f * 1024.0f)) + 200) > memory_checker::get_mb_left())
    {
        PRINTLN("not enough available memory (free: {}MB, model: {:.4f}MB) for path: {}",
            memory_checker::get_mb_left(), ((float)(model_size / (1024.0f * 1024.0f))), a_path);
        mesh.bad_load = true;
        mesh.ram_load_status = ELoadStatus::FailedLoadNoSpace;
        return mesh;
    }
    bool has_color = false;
    bool has_3d_uvs = false;
    mesh.colors.emplace_back(std::vector<glm::vec4>());
    mesh.uvs.emplace_back(std::vector<glm::vec3>());
    size_t num_pos = 0;
    size_t num_nrm = 0;
    size_t num_uv = 0;
    size_t num_f = 0;

    std::vector<glm::vec3> v_pos;
    std::vector<glm::vec4> v_col;
    std::vector<glm::vec3> v_nrm;
    std::vector<glm::vec3> v_uv;
    std::vector<ObjFaceVertex> v_indicies;

    glm::vec3 pos;
    glm::vec4 col;
    glm::vec3 nrm;
    glm::vec3 uv;
    ObjFace obj_face;
    GLuint unique_index;

    using vertexKey = std::tuple<GLuint, GLuint, GLuint>;
    std::map<vertexKey, GLuint> v_map;

    GLuint index = 0;
    while (std::getline(file, line))
    {
        std::istringstream string_stream(line);
        std::string prefix;
        string_stream >> prefix;
        if (prefix == "v")
        {
            num_pos++;
            string_stream >> pos.x >> pos.y >> pos.z >> col.r >> col.g >> col.b;
            col.r = std::max(0.0f, std::min(1.0f, col.r));
            col.g = std::max(0.0f, std::min(1.0f, col.g));
            col.b = std::max(0.0f, std::min(1.0f, col.b));
            has_color |= (col.r + col.g + col.b) > 0;
            v_pos.push_back(pos);
            v_col.push_back(glm::vec4(col.r, col.g, col.b, 1.0f));
        }
        else if (prefix == "vn")
        {
            num_nrm++;

            string_stream >> nrm.x >> nrm.y >> nrm.z;
            nrm.x = std::max(-1.0f, std::min(1.0f, nrm.x));
            nrm.y = std::max(-1.0f, std::min(1.0f, nrm.y));
            nrm.z = std::max(-1.0f, std::min(1.0f, nrm.z));
            v_nrm.push_back(nrm);
        }
        else if (prefix == "vt")
        {
            num_uv++;

            string_stream >> uv.x >> uv.y >> uv.z;
            uv.x = std::max(0.0f, std::min(1.0f, uv.x));
            uv.y = std::max(0.0f, std::min(1.0f, uv.y));
            uv.z = std::max(0.0f, std::min(1.0f, uv.z));
            has_3d_uvs |= uv.z > 0;
            v_uv.push_back(uv);
        }
        else if (prefix == "f")
        {
            num_f++;

            line.erase(0, 1);

            std::istringstream faces_string_stream(line);
            std::string face_string;

            size_t num_indices = 0;
            while (faces_string_stream >> face_string)
            {
                //index++;
                num_indices++;
                ObjFaceVertex obj_face_vert; // should be making these once, ...
                std::replace(face_string.begin(), face_string.end(), '/', ' ');
                std::istringstream face_stream(face_string);
                face_stream >> obj_face_vert.pos >> obj_face_vert.uv >> obj_face_vert.nrm;

                vertexKey key(obj_face_vert.pos, obj_face_vert.uv, obj_face_vert.nrm);
                auto it = v_map.find(key);
                if (it == v_map.end())
                {
                    v_map[key] = index;
                    unique_index = index++;
                }
                else
                {
                    unique_index = it->second;
                }
                mesh.indices.push_back(unique_index);

                if (unique_index == mesh.positions.size())
                {

                    mesh.positions.push_back(v_pos[obj_face_vert.pos - 1]);
                    if (has_color)
                    {
                        mesh.colors[0].push_back(v_col[obj_face_vert.pos - 1]);
                    }
                    if (v_nrm.size() > 0)
                        mesh.normals.push_back(v_nrm[obj_face_vert.nrm - 1]);
                    if (v_uv.size() > 0)
                        mesh.uvs[0].push_back(v_uv[obj_face_vert.uv - 1]);
                }

            }
            if (num_indices > 3)
            {
                PRINTLN("OBJ not triangulized");
                mesh.bad_load = true;
                mesh.ram_load_status = ELoadStatus::FailedLoadBadModel;
                return mesh;
            }
        }
    }

    if (has_color)
    {
        mesh.colors_dimensions.emplace_back(std::vector<uint8_t>().emplace_back(3));
    }
    else
    {
        mesh.colors[0].clear();
        mesh.colors.clear();
    }
    if (mesh.uvs[0].size() > 0)
    {
        mesh.uvs_dimensions.emplace_back(has_3d_uvs ? 3 : 2);
    }
    else
    {
        mesh.uvs.clear();
    }
    title = a_path + " finished started at";
    time_now.print_time(title);
    timer.stop();
    mesh.path = a_path;
    PRINTLN("pos: {}, nrm: {}, uv: {}, face: {}", num_pos, num_nrm, num_uv, num_f);
    PRINTLN("time to load {}: {}ms", a_path, timer.get_time_ms());
    mesh.ram_load_status = ELoadStatus::Loaded;
    return mesh;
}

void ModelLoader::load_texture_from_file(TextureData& a_texture_data)
{
    if (a_texture_data.path == "")
    {
        PRINTLN("No path assigned");
        a_texture_data.texture_ram_status = ELoadStatus::NotLoaded;
        return;
    }
    // load and generate the texture
    int temp_width, temp_height, temp_num_comps;
    stbi_uc* raw_ptr = stbi_load(a_texture_data.hashed_path.string.c_str(), &temp_width, &temp_height, &temp_num_comps, 0);
    if (raw_ptr == nullptr)
    {
        PRINTLN("Failed Generate STBI data");
        a_texture_data.texture_ram_status = ELoadStatus::FailedLoadOpen;
        return;
    }
    if (temp_num_comps == 1) a_texture_data.color_format = GL_RED;
    else if (temp_num_comps == 2) a_texture_data.color_format = GL_RG;
    else if (temp_num_comps == 3) a_texture_data.color_format = GL_RGB;
    else if (temp_num_comps == 4) a_texture_data.color_format = GL_RGBA;
    else
    {
        std::println("Failed to load texture");
        std::println("loaded texture at {}", a_texture_data.texture_id);
        a_texture_data.texture_ram_status = ELoadStatus::FailedLoadBadModel;
        stbi_image_free(raw_ptr);
        return;
    }
    size_t data_size = static_cast<size_t>(temp_width) * temp_height * temp_num_comps;
    a_texture_data.texture_data = std::shared_ptr<stbi_uc>(new stbi_uc[data_size]);
    std::copy_n(raw_ptr, data_size, a_texture_data.texture_data.get());
    stbi_image_free(raw_ptr);

    a_texture_data.width = temp_width;
    a_texture_data.height = temp_height;
    a_texture_data.texture_ram_status = ELoadStatus::Loaded;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, bool a_mipmap)
{
    TextureData texture_data;
    texture_data.hashed_path = hashed_string_64(a_path.c_str());
    texture_data.path = a_path;
    texture_data.generate_mipmap = a_mipmap;
    load_texture_from_file(texture_data);
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap)
{
    TextureData texture_data;
    texture_data.hashed_path = hashed_string_64(a_path.c_str());
    texture_data.path = a_path;
    texture_data.wrap_x = a_wrap_x;
    texture_data.wrap_y = a_wrap_y;
    texture_data.generate_mipmap = a_mipmap;
    load_texture_from_file(texture_data);
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap)
{
    TextureData texture_data;
    texture_data.hashed_path = hashed_string_64(a_path.c_str());
    texture_data.path = a_path;
    texture_data.wrap_x = a_wrap_x;
    texture_data.wrap_y = a_wrap_y;
    texture_data.generate_mipmap = a_mipmap;
    texture_data.filtering_min = a_filtering_min;
    texture_data.filtering_mag = a_fintering_mag;
    load_texture_from_file(texture_data);
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, const GLenum a_mipmap_filtering, bool a_mipmap)
{
    TextureData texture_data;
    texture_data.hashed_path = hashed_string_64(a_path.c_str());
    texture_data.path = a_path;
    texture_data.wrap_x = a_wrap_x;
    texture_data.wrap_y = a_wrap_y;
    texture_data.generate_mipmap = a_mipmap;
    texture_data.filtering_min = a_filtering_min;
    texture_data.filtering_mag = a_fintering_mag;
    texture_data.mipmap_filtering = a_mipmap_filtering;
    load_texture_from_file(texture_data);
    return texture_data;
}
