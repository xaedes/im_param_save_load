#pragma once

#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime> 
#include <chrono> 

#include "imgui.h"
#include "imgui_candy/imgui_candy.h"

#include "im_param/detail/cpp11_template_logic.h"
#include "im_param/detail/backend.h"
#include "im_param/backends/imgui_backend.h"
#include "im_param/backends/json_backend.h"
#include "im_param_undo_redo/linear_history.h"

namespace im_param_save_load {

    template<
        class GuiBackend_t = im_param::ImGuiBackend,
        class SerializerBackend_t = im_param::JsonSerializerBackend,
        class DeserializerBackend_t = im_param::JsonDeserializerBackend
    >
    struct SaveLoadBackend {
        bool changed = false;

        using GuiBackend = GuiBackend_t;
        using SerializerBackend = SerializerBackend_t;
        using DeserializerBackend = DeserializerBackend_t;
        template <class... Args> using TypeHolder = typename im_param::TypeHolder<Args...>;
        using HierarchyType = typename im_param::HierarchyType;
        using Backend = typename im_param::Backend;

        SaveLoadBackend()
        {}

        std::string path = "./";
        std::string default_filename = "default.json";
        std::vector<std::string> slot_filenames{
            "slot0_ananas_ape.json", 
            "slot1_banana_buffalo.json", 
            "slot2_carrot_cow.json", 
            "slot3_dill_duck.json", 
            "slot4_eggplant_eagle.json",
            "slot5_flower_fox.json",
            "slot6_garlic_gecko.json",
            "slot7_hay_hawk.json",
            "slot8_ivy_ibex.json",
            "slot9_juniper_jackal.json",
        };

        GuiBackend gui_backend;
        SerializerBackend serializer_backend;
        DeserializerBackend deserializer_backend;


        template <class value_type, class... Args>
        std::string serialize(const std::string& name, value_type& value, Args&&... args)
        {
            serializer_backend.clear();
            serializer_backend.parameter(name, value, std::forward<Args>(args)...);
            return serializer_backend.serialized();;
        }

        template <class value_type, class... Args>
        bool deserialize(const std::string& serialized, const std::string& name, value_type& value, Args&&... args)
        {
            deserializer_backend.clear();
            deserializer_backend.changed = false;
            deserializer_backend.deserialize(serialized);
            deserializer_backend.parameter(name, value, std::forward<Args>(args)...);
            return deserializer_backend.changed;
        }

        template <class value_type, class... Args>
        void save(const std::string& filename, const std::string& name, value_type& value, Args&&... args)
        {
            std::ofstream ofs(filename.c_str());
            ofs << serialize(name, value, std::forward<Args>(args)...);
        }

        template <class value_type, class... Args>
        bool load(const std::string& filename, const std::string& name, value_type& value, Args&&... args)
        {
            std::ifstream ifs(filename.c_str());
            if (!ifs.good()) return false;
            static std::stringstream read_buffer;
            read_buffer.clear(); // clear flags 
            read_buffer.str("");
            read_buffer << ifs.rdbuf();
            return deserialize(read_buffer.str(), name, value, std::forward<Args>(args)...);
        }

        std::string str_current_date_time()
        {
            using std::chrono::system_clock;
            std::time_t tt = system_clock::to_time_t (system_clock::now());
            static std::stringstream ss;
            ss.str("");
            ss  << std::put_time(std::localtime(&tt), "%Y_%m_%d_%H_%M_%S");
            ss << ".json";
            return ss.str();
        }

        bool file_exists(const std::string& fn)
        {
            std::ifstream ifs(fn.c_str());
            return ifs.good();
        }

        // template<class... Args, std::enable_if_t<Backend::is_specialized<value_type>::value, bool> = true>
        template<class value_type, class... Args/*, std::enable_if_t<Backend::is_specialized<value_type>::value, bool> = true*/>
        SaveLoadBackend& parameter(const std::string& name, value_type& value, Args&&... args)
        {
            this->begin(name, value, std::forward<Args>(args)...);
            gui_backend.changed = false;
            bool changed_by_user = gui_backend.parameter(
                name, value, std::forward<Args>(args)...
            ).changed;
            this->end(changed_by_user, name, value, std::forward<Args>(args)...);
            return *this;
        }

        // template<typename U, class... Args, std::enable_if_t<!Backend::is_specialized<value_type>::value, bool> = true>
        template<class value_type, typename U, class... Args /*, std::enable_if_t<!Backend::is_specialized<value_type>::value, bool> = true */>
        SaveLoadBackend& parameter(const std::string& name, value_type& value, const TypeHolder<U>& typeholder, Args&&... args)
        {
            this->begin(name, value, typeholder, typeholder, std::forward<Args>(args)...);
            gui_backend.changed = false;
            bool changed_by_user = gui_backend.parameter(
                name, value, typeholder, std::forward<Args>(args)...
            ).changed;
            this->end(changed_by_user, name, value, typeholder, std::forward<Args>(args)...);
            return *this;
        }

        template <class value_type, class... Args>
        void begin(const std::string& name, value_type& value, Args&&... args)
        {
            static std::string filename = "";
            
            if (ImGuiButton("Load"))
            {
                changed |= load(filename, name, value, std::forward<Args>(args)...);
            }
            ImGui::SameLine();
            static bool dont_ask_me_next_time = false;
            if (ImGuiButton("Save"))
            {
                if (file_exists(filename) && !dont_ask_me_next_time)
                {
                    ImGui::OpenPopup(ImGuiCandy::append_id("Confirm overwrite",&filename).c_str());
                }
                else
                {
                    save(filename, name, value, std::forward<Args>(args)...);
                }
            }
            if (ImGui::BeginPopupModal(ImGuiCandy::append_id("Confirm overwrite",&filename).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("This file already exists. Do you want to overwrite it?");
                ImGui::Separator();

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::Checkbox(
                    ImGuiCandy::append_id("Don't ask me next time",&dont_ask_me_next_time).c_str(), 
                    &dont_ask_me_next_time
                );
                ImGui::PopStyleVar();

                if (ImGui::Button("Confirm", ImVec2(120, 0)))
                {
                    save(filename, name, value, std::forward<Args>(args)...);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) 
                    ImGui::CloseCurrentPopup();
                ImGui::SetItemDefaultFocus();
                ImGui::EndPopup();
            }

            static int combo_current = 0;
            static std::vector<const char*> combo_items;
            struct UserData {
                int* current;
                std::vector<const char*>* items;
            };
            UserData user_data{
                &combo_current,
                &combo_items
            };
            ImGui::InputText(
                ImGuiCandy::append_id("",&filename).c_str(), 
                &filename,
                ImGuiInputTextFlags_CallbackHistory, 
                [](ImGuiInputTextCallbackData* data)
                {
                    UserData user_data = *static_cast<UserData*>(data->UserData);
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
                    {
                        if (data->EventKey == ImGuiKey_UpArrow)
                        {
                            *user_data.current = (*user_data.current - 1) % user_data.items->size();
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, (*user_data.items)[*user_data.current]);
                            data->SelectAll();
                        }
                        else if (data->EventKey == ImGuiKey_DownArrow)
                        {
                            *user_data.current = (*user_data.current + 1) % user_data.items->size();
                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, (*user_data.items)[*user_data.current]);
                            data->SelectAll();
                        }
                    }
                    return 0;
                },
                &user_data
                // Func::Callback
            );
            
            static std::string combo_filename = "";
            static std::string filename_datetime = "";
            filename_datetime = str_current_date_time();
            combo_items.clear();
            combo_items.push_back(default_filename.c_str());
            combo_items.push_back(filename_datetime.c_str());
            for (int i=0; i<slot_filenames.size(); ++i)
            {
                combo_items.push_back(slot_filenames[i].c_str());
            }
            ImGui::SameLine();
            if (ImGui::BeginCombo(
                ImGuiCandy::append_id("filename",&combo_current).c_str(),
                "", 
                ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_NoPreview
            ))
            {
                for (int i = 0; i < combo_items.size(); i++)
                {
                    const bool is_selected = (combo_current == i);
                    if (ImGui::Selectable(combo_items[i], is_selected))
                    {
                        combo_current = i;
                        filename = combo_items[combo_current];
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            // if (ImGui::Combo(
            //     ImGuiCandy::append_id("filenamed",&combo_current).c_str(),
            //     &combo_current,
            //     combo_items.data(),
            //     combo_items.size()
            // ))
            // {
            //     filename = combo_items[combo_current];
            // }
        }

        template <class value_type, class... Args>
        void end(bool changed_by_user, const std::string& name, value_type& value, Args&&... args)
        {
            if (changed_by_user)
            {
                // auto serizalized = serialize(name, value, std::forward<Args>(args)...);
                // history.push_back(serizalized);
                changed |= true;
            }

        }

    };

} // namespace im_param_save_load
