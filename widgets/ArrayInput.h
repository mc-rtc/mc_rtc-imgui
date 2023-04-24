#pragma once

#include "Widget.h"

namespace mc_rtc::imgui
{

struct ArrayInput : public Widget
{
  inline ArrayInput(Client & client, const ElementId & id) : Widget(client, id) {}

  ~ArrayInput() override = default;

  inline void data(const std::vector<std::string> & labels, const Eigen::VectorXd & data)
  {
    if(!busy_)
    {
      labels_ = labels;
      data_ = data;
    }
  }

  inline void draw2D() override
  {
    int flags = busy_ ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_ReadOnly;
    double * source = busy_ ? buffer_.data() : data_.data();
    bool edit_done_ = false;
    ImGui::Text("%s", id.name.c_str());
    ImGui::SameLine();
    edit_done_ = ImGui::Button(label(busy_ ? "Done" : "Edit").c_str());
    ImGui::Columns(data_.size());
    if(labels_.size())
    {
      for(size_t i = 0; i < std::min<size_t>(labels_.size(), data_.size()); ++i)
      {
        const auto & l = labels_[i];
        ImGui::Text("%s", l.c_str());
        ImGui::NextColumn();
      }
      for(size_t i = labels_.size(); i < static_cast<size_t>(data_.size()); ++i)
      {
        ImGui::NextColumn();
      }
    }
    for(int i = 0; i < data_.size(); ++i)
    {
      ImGui::InputDouble(label("", i).c_str(), &source[i], 0.0, 0.0, "%.6g", flags);
      edit_done_ = edit_done_
                   || (ImGui::IsItemDeactivatedAfterEdit()
                       && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeyPadEnter)));
      ImGui::NextColumn();
    }
    ImGui::Columns(1);
    if(edit_done_)
    {
      if(busy_)
      {
        if(buffer_ != data_)
        {
          data_ = buffer_;
          client.send_request(id, data_);
        }
        busy_ = false;
      }
      else
      {
        buffer_ = data_;
        busy_ = true;
      }
    }
  }

private:
  bool busy_ = false;
  std::vector<std::string> labels_;
  Eigen::VectorXd data_;
  Eigen::VectorXd buffer_;
};

} // namespace mc_rtc::imgui
