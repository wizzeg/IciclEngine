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
    std::string title = a_path + " - mesh loading started at";
    timer.start();
    time_now.print_time(title);
    std::vector<ObjVertex> vertices;
    std::ifstream file(a_path);
    std::string line;

    MeshData mesh;
    //mesh.started_load = true;
    mesh.contents->hashed_path = hashed_string_64(a_path.c_str());
    mesh.ram_load_status = ELoadStatus::StartedLoad;
    if (!file.is_open())
    {
        PRINTLN("failed open file at: {}", a_path);
        //mesh.bad_load = true;
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
        //mesh.bad_load = true;
        mesh.ram_load_status = ELoadStatus::FailedLoadNoSpace;
        return mesh;
    }
    bool has_color = false;
    bool has_3d_uvs = false;
    mesh.contents->colors.emplace_back(std::vector<glm::vec4>());
    mesh.contents->uvs.emplace_back(std::vector<glm::vec3>());
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
                mesh.contents->indices.push_back(unique_index);

                if (unique_index == mesh.contents->positions.size())
                {

                    mesh.contents->positions.push_back(v_pos[obj_face_vert.pos - 1]);
                    if (has_color)
                    {
                        mesh.contents->colors[0].push_back(v_col[obj_face_vert.pos - 1]);
                    }
                    if (v_nrm.size() > 0)
                    {
                        mesh.contents->normals.push_back(v_nrm[obj_face_vert.nrm - 1]);
                    }
                    if (v_uv.size() > 0)
                    {
                        mesh.contents->uvs[0].push_back(v_uv[obj_face_vert.uv - 1]);
                    }
                        
                }

            }
            if (num_indices > 3)
            {
                PRINTLN("OBJ not triangulized");
                //mesh.bad_load = true;
                mesh.ram_load_status = ELoadStatus::FailedLoadBadModel;
                return mesh;
            }
        }
    }

    if (has_color)
    {
        mesh.contents->colors_dimensions.emplace_back(std::vector<uint8_t>().emplace_back(3));
    }
    else
    {
        mesh.contents->colors[0].clear();
        mesh.contents->colors.clear();
    }
    if (mesh.contents->uvs[0].size() > 0)
    {
        mesh.contents->uvs_dimensions.emplace_back(has_3d_uvs ? 3 : 2);
    }
    else
    {
        mesh.contents->uvs.clear();
    }


    mesh.num_indicies = (GLsizei)mesh.contents->indices.size();
    mesh.hash = mesh.contents->hashed_path.hash;

    ///////////////////////////////////////
    // create tangents and bitangets
    create_tangent_bitangent(mesh);

    title = a_path + " - mesh finished started at";
    time_now.print_time(title);
    timer.stop();
    //mesh.path = a_path;
    PRINTLN("pos: {}, nrm: {}, uv: {}, face: {}", num_pos, num_nrm, num_uv, num_f);
    PRINTLN("time to load mesh {}: {}ms", a_path, timer.get_time_ms());
    mesh.ram_load_status = ELoadStatus::Loaded;

    return mesh;
}

void ModelLoader::load_texture_from_file(TextureData& a_texture_data)
{
    stbi_set_flip_vertically_on_load(true);
    //a_texture_data.contents->path = a_texture_data.hashed_path.string;
    if (a_texture_data.contents->hashed_path.string == " ")
    {
        PRINTLN("No path assigned");
        a_texture_data.texture_ram_status = ELoadStatus::NotLoaded;
        return;
    }
    // load and generate the texture
    int temp_width, temp_height, temp_num_comps;
    stbi_uc* raw_ptr = stbi_load(a_texture_data.contents->hashed_path.string.c_str(), &temp_width, &temp_height, &temp_num_comps, 0);
    if (raw_ptr == nullptr)
    {
        PRINTLN("Failed Generate STBI data");
        a_texture_data.texture_ram_status = ELoadStatus::FailedLoadOpen;
        return;
    }
    if (temp_num_comps == 1) a_texture_data.contents->color_format = GL_RED;
    else if (temp_num_comps == 2) a_texture_data.contents->color_format = GL_RG;
    else if (temp_num_comps == 3) a_texture_data.contents->color_format = GL_RGB;
    else if (temp_num_comps == 4) a_texture_data.contents->color_format = GL_RGBA;
    else
    {
        std::println("Failed to load texture");
        std::println("loaded texture at {}", a_texture_data.texture_id);
        a_texture_data.texture_ram_status = ELoadStatus::FailedLoadBadModel;
        stbi_image_free(raw_ptr);
        return;
    }
    size_t data_size = static_cast<size_t>(temp_width) * temp_height * temp_num_comps;
    a_texture_data.contents->texture_data = std::shared_ptr<stbi_uc>(new stbi_uc[data_size]);
    std::copy_n(raw_ptr, data_size, a_texture_data.contents->texture_data.get());
    stbi_image_free(raw_ptr);

    a_texture_data.contents->width = temp_width;
    a_texture_data.contents->height = temp_height;
    a_texture_data.hash = a_texture_data.contents->hashed_path.hash;
    a_texture_data.texture_ram_status = ELoadStatus::Loaded;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, bool a_mipmap)
{
    HighResolutionTimer timer;
    TimeNow time_now;
    std::string title = a_path + " - texture loading started at";
    timer.start();
    time_now.print_time(title);
    TextureData texture_data;
    if (((float)(256) + 200) > memory_checker::get_mb_left())
    {
        PRINTLN("not enough available memory (free: {}MB, model: {:.4f}MB) for path: {}",
            memory_checker::get_mb_left(), ((float)(256 / (1024.0f * 1024.0f))), a_path);
        texture_data.texture_ram_status = ELoadStatus::FailedLoadNoSpace;
        return texture_data;
    }
    texture_data.contents->hashed_path = hashed_string_64(a_path.c_str());
    //texture_data.path = a_path;
    texture_data.contents->generate_mipmap = a_mipmap;
    load_texture_from_file(texture_data);
    time_now.print_time(title);
    timer.stop();
    PRINTLN("time to load texture {}: {}ms", a_path, timer.get_time_ms());
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, bool a_mipmap)
{
    HighResolutionTimer timer;
    TimeNow time_now;
    std::string title = a_path + " - texture loading started at";
    timer.start();
    time_now.print_time(title);
    TextureData texture_data;
    if (((float)(256) + 200) > memory_checker::get_mb_left())
    {
        PRINTLN("not enough available memory (free: {}MB, model: {:.4f}MB) for path: {}",
            memory_checker::get_mb_left(), ((float)(256 / (1024.0f * 1024.0f))), a_path);
        texture_data.texture_ram_status = ELoadStatus::FailedLoadNoSpace;
        return texture_data;
    }
    texture_data.contents->hashed_path = hashed_string_64(a_path.c_str());
    //texture_data.contents->path = a_path;
    texture_data.contents->wrap_x = a_wrap_x;
    texture_data.contents->wrap_y = a_wrap_y;
    texture_data.contents->generate_mipmap = a_mipmap;
    load_texture_from_file(texture_data);
    title = a_path + " - texture finished started at";
    time_now.print_time(title);
    timer.stop();
    PRINTLN("time to load texture {}: {}ms", a_path, timer.get_time_ms());
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, bool a_mipmap)
{
    HighResolutionTimer timer;
    TimeNow time_now;
    std::string title = a_path + " - texture loading started at";
    timer.start();
    time_now.print_time(title);
    TextureData texture_data;
    if (((float)(256) + 200) > memory_checker::get_mb_left())
    {
        PRINTLN("not enough available memory (free: {}MB, model: {:.4f}MB) for path: {}",
            memory_checker::get_mb_left(), ((float)(256 / (1024.0f * 1024.0f))), a_path);
        texture_data.texture_ram_status = ELoadStatus::FailedLoadNoSpace;
        return texture_data;
    }
    texture_data.contents->hashed_path = hashed_string_64(a_path.c_str());
    //texture_data.path = a_path;
    texture_data.contents->wrap_x = a_wrap_x;
    texture_data.contents->wrap_y = a_wrap_y;
    texture_data.contents->generate_mipmap = a_mipmap;
    texture_data.contents->filtering_min = a_filtering_min;
    texture_data.contents->filtering_mag = a_fintering_mag;
    load_texture_from_file(texture_data);
    title = a_path + " - texture finished started at";
    time_now.print_time(title);
    timer.stop();
    PRINTLN("time to load texture {}: {}ms", a_path, timer.get_time_ms());
    return texture_data;
}

TextureData ModelLoader::load_texture_from_file(const std::string a_path, const GLenum a_wrap_x, const GLenum a_wrap_y, const GLenum a_filtering_min, const GLenum a_fintering_mag, const GLenum a_mipmap_filtering, bool a_mipmap)
{
    HighResolutionTimer timer;
    TimeNow time_now;
    std::string title = a_path + " - texture loading started at";
    timer.start();
    time_now.print_time(title);
    TextureData texture_data;
    if (((float)(256) + 200) > memory_checker::get_mb_left())
    {
        PRINTLN("not enough available memory (free: {}MB, model: {:.4f}MB) for path: {}",
            memory_checker::get_mb_left(), ((float)(256 / (1024.0f * 1024.0f))), a_path);
        texture_data.texture_ram_status = ELoadStatus::FailedLoadNoSpace;
        return texture_data;
    }
    texture_data.contents->hashed_path = hashed_string_64(a_path.c_str());
    //texture_data.path = a_path;
    texture_data.contents->wrap_x = a_wrap_x;
    texture_data.contents->wrap_y = a_wrap_y;
    texture_data.contents->generate_mipmap = a_mipmap;
    texture_data.contents->filtering_min = a_filtering_min;
    texture_data.contents->filtering_mag = a_fintering_mag;
    texture_data.contents->mipmap_filtering = a_mipmap_filtering;
    load_texture_from_file(texture_data);
    title = a_path + " - texture finished started at";
    time_now.print_time(title);
    timer.stop();
    PRINTLN("time to load texture {}: {}ms", a_path, timer.get_time_ms());
    return texture_data;
}

void ModelLoader::create_tangent_bitangent(MeshData& a_mesh_data)
{
    auto& mesh_contents = a_mesh_data.contents;
    size_t num_indices = (size_t)a_mesh_data.num_indicies;
    size_t num_vertices = mesh_contents->positions.size();
    
    auto& indices = mesh_contents->indices;
    auto& positions = mesh_contents->positions;
    auto& normals = mesh_contents->normals;
    auto& uvs = mesh_contents->uvs;
    
    // return checks
    if (uvs.empty() || normals.empty()) return;
    else if (uvs[0].empty()) return;

    std::vector<glm::vec3> tangents(num_vertices, glm::vec3(0));
    std::vector<glm::vec4> tangents_handed(num_vertices, glm::vec4(0));
    std::vector<glm::vec3> bitangents(num_vertices, glm::vec3(0));

    for (size_t i = 2; i < num_indices; i+=3)
    {
        // for each set of 3 indices, we use to vertexes for the face to determine the tangents and bitangents

        // first face indices
        auto index0 = indices[i-2];
        auto index1 = indices[i-1];
        auto index2 = indices[i];

        glm::vec3 edge1 = positions[index1] - positions[index0];
        glm::vec3 edge2 = positions[index2] - positions[index0];

        glm::vec2 delta_uv1 = glm::vec2(uvs[0][index1].x, uvs[0][index1].y) - glm::vec2(uvs[0][index0].x, uvs[0][index0].y);
        glm::vec2 delta_uv2 = glm::vec2(uvs[0][index2].x, uvs[0][index2].y) - glm::vec2(uvs[0][index0].x, uvs[0][index0].y);

        float determinant = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);

        glm::vec3 tangent = determinant * (delta_uv2.y * edge1 - delta_uv1.y * edge2);
        glm::vec3 bitanget = determinant * (-delta_uv2.x * edge1 + delta_uv1.x * edge2);

        tangents[index0] += tangent;
        tangents[index1] += tangent;
        tangents[index2] += tangent;
        bitangents[index0] += bitanget;
        bitangents[index1] += bitanget;
        bitangents[index2] += bitanget;
    }
    for (size_t i = 0; i < num_vertices; i++)
    {
        tangents[i] = glm::normalize(tangents[i] - normals[i] * glm::dot(normals[i], tangents[i]));
        //bitangents[i] = glm::normalize(bitangents[i]);
        glm::vec3 orthogonalization = glm::cross(normals[i], tangents[i]);
        float handedness = glm::dot(bitangents[i], orthogonalization) < 0.0f? -1.0f : 1.0f;

        tangents_handed[i] = glm::vec4(tangents[i], handedness);
    }
    mesh_contents->tangents = tangents_handed;
    //mesh_contents->bitangents = bitangents;
}
