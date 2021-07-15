#include "Client.h"

#include "widgets/ArrayInput.h"
#include "widgets/ArrayLabel.h"
#include "widgets/Button.h"
#include "widgets/Checkbox.h"
#include "widgets/ComboInput.h"
#include "widgets/DataComboInput.h"
#include "widgets/Form.h"
#include "widgets/IntegerInput.h"
#include "widgets/Label.h"
#include "widgets/NumberInput.h"
#include "widgets/NumberSlider.h"
#include "widgets/Schema.h"
#include "widgets/StringInput.h"
#include "widgets/Table.h"

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

namespace mc_rtc::imgui
{

Client::Client() : mc_control::ControllerClient()
{
  std::string socket = fmt::format("ipc://{}", (bfs::temp_directory_path() / "mc_rtc_").string());
  connect(socket + "pub.ipc", socket + "rep.ipc");
  timeout(3.0);
}

void Client::update()
{
  run(buffer_, t_last_);
}

void Client::draw2D(ImVec2 windowSize)
{
  auto left_margin = 15;
  auto top_margin = 50;
  auto bottom_margin = 50;
  auto width = windowSize.x - left_margin;
  auto height = windowSize.y - top_margin - bottom_margin;
  if(!root_.empty())
  {
    ImGui::SetNextWindowPos(ImVec2(left_margin, top_margin), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(0.4f * width, 0.7f * height), ImGuiCond_FirstUseEver);
    ImGui::Begin("mc_rtc");
    root_.draw2D();
    ImGui::End();
  }
}

void Client::draw3D()
{
  root_.draw3D();
}

void Client::started()
{
  root_.started();
}

void Client::stopped()
{
  root_.stopped();
}

void Client::clear()
{
  root_.categories.clear();
  root_.widgets.clear();
}

/** We rely on widgets to create categories */
void Client::category(const std::vector<std::string> &, const std::string &) {}

void Client::label(const ElementId & id, const std::string & txt)
{
  widget<Label>(id).data(txt);
}

void Client::array_label(const ElementId & id, const std::vector<std::string> & labels, const Eigen::VectorXd & data)
{
  widget<ArrayLabel>(id).data(labels, data);
}

void Client::button(const ElementId & id)
{
  widget<Button>(id);
}

void Client::checkbox(const ElementId & id, bool state)
{
  widget<Checkbox>(id).data(state);
}

void Client::string_input(const ElementId & id, const std::string & data)
{
  widget<StringInput>(id).data(data);
}

void Client::integer_input(const ElementId & id, int data)
{
  widget<IntegerInput>(id).data(data);
}

void Client::number_input(const ElementId & id, double data)
{
  widget<NumberInput>(id).data(data);
}

void Client::number_slider(const ElementId & id, double data, double min, double max)
{
  widget<NumberSlider>(id).data(data, min, max);
}

void Client::array_input(const ElementId & id, const std::vector<std::string> & labels, const Eigen::VectorXd & data)
{
  widget<ArrayInput>(id).data(labels, data);
}

void Client::combo_input(const ElementId & id, const std::vector<std::string> & values, const std::string & data)
{
  widget<ComboInput>(id).data(values, data);
}

void Client::data_combo_input(const ElementId & id, const std::vector<std::string> & values, const std::string & data)
{
  widget<DataComboInput>(id).data(values, data);
}

void Client::table_start(const ElementId & id, const std::vector<std::string> & header)
{
  widget<Table>(id).start(header);
}

void Client::table_row(const ElementId & id, const std::vector<std::string> & data)
{
  widget<Table>(id).row(data);
}

void Client::table_end(const ElementId & id)
{
  widget<Table>(id).end();
}

void Client::form(const ElementId & id)
{
  widget<Form>(id);
}

void Client::form_checkbox(const ElementId & id, const std::string & name, bool required, bool default_)
{
  widget<Form>(id).widget<form::Checkbox>(name, required, default_);
}

void Client::form_integer_input(const ElementId & id, const std::string & name, bool required, int default_)
{
  widget<Form>(id).widget<form::IntegerInput>(name, required, default_);
}

void Client::form_number_input(const ElementId & id, const std::string & name, bool required, double default_)
{
  widget<Form>(id).widget<form::NumberInput>(name, required, default_);
}

void Client::form_string_input(const ElementId & id,
                               const std::string & name,
                               bool required,
                               const std::string & default_)
{
  widget<Form>(id).widget<form::StringInput>(name, required, default_);
}

void Client::form_array_input(const ElementId & id,
                              const std::string & name,
                              bool required,
                              const Eigen::VectorXd & default_,
                              bool fixed_size)
{
  widget<Form>(id).widget<form::ArrayInput>(name, required, default_, fixed_size);
}

void Client::form_combo_input(const ElementId & id,
                              const std::string & name,
                              bool required,
                              const std::vector<std::string> & values,
                              bool send_index)
{
  widget<Form>(id).widget<form::ComboInput>(name, required, values, send_index);
}

void Client::form_data_combo_input(const ElementId & id,
                                   const std::string & name,
                                   bool required,
                                   const std::vector<std::string> & ref,
                                   bool send_index)
{
  widget<Form>(id).widget<form::DataComboInput>(name, required, ref, send_index);
}

void Client::schema(const ElementId & id, const std::string & schema)
{
  widget<Schema>(id).data(schema);
}

auto Client::getCategory(const std::vector<std::string> & category) -> Category &
{
  std::reference_wrapper<Category> out(root_);
  for(size_t i = 0; i < category.size(); ++i)
  {
    auto & cat = out.get();
    auto & next = category[i];
    auto it = std::find_if(cat.categories.begin(), cat.categories.end(), [&](auto & c) { return c->name == next; });
    if(it != cat.categories.end())
    {
      out = std::ref(*it->get());
    }
    else
    {
      out = *cat.categories.emplace_back(std::make_unique<Category>(next, cat.depth + 1));
    }
  }
  return out.get();
}

} // namespace mc_rtc::imgui
