#pragma once

#include "../../Client.h"
#include "../../Widget.h"
#include "../IndentedSeparator.h"

#include <memory>
#include <optional>

namespace mc_rtc::imgui
{

namespace form
{

struct Widget;
using WidgetPtr = std::unique_ptr<Widget>;

struct ObjectWidget;
using ObjectWidgetPtr = std::unique_ptr<ObjectWidget>;

struct OneOfWidget;
using OneOfWidgetPtr = std::unique_ptr<OneOfWidget>;

struct Widget
{
  Widget(const ::mc_rtc::imgui::Widget & parent, const std::string & name)
  : parent_(parent), name_(name), id_(next_id_++)
  {
  }

  virtual ~Widget() = default;

  /** Should return a copy of the form widget
   *
   * This is used to create array of objects from template objects
   *
   * \param parent Parent container initiating the copy if any
   */
  virtual WidgetPtr clone(ObjectWidget * parent) const = 0;

  virtual bool ready() = 0;

  inline void lock()
  {
    locked_ = true;
  }

  inline void unlock()
  {
    locked_ = false;
  }

  inline bool locked() const noexcept
  {
    return locked_;
  }

  void draw()
  {
    parent_.client.enable_bold_font();
    ImGui::Text("%s", name().c_str());
    parent_.client.disable_bold_font();
    if(locked_)
    {
      ImGui::SameLine();
      if(ImGui::Button(label("Reset").c_str()))
      {
        locked_ = false;
      }
    }
    draw_();
  }

  virtual void draw_() = 0;

  virtual void draw3D() {}

  /** A form widget is trivial if it doesn't contain other widgets */
  inline virtual bool trivial() const
  {
    return true;
  }

  inline virtual std::string value()
  {
    mc_rtc::log::error_and_throw<std::runtime_error>("::value() is not implemented for this form element");
  }

  virtual void collect(mc_rtc::Configuration & out) = 0;

  template<typename T = const char *>
  inline std::string label(std::string_view label, T suffix = "")
  {
    return fmt::format("{}##{}{}{}{}_{}", label, parent_.id.category, parent_.id.name, name_, suffix, id_);
  }

  inline std::string name() const
  {
    size_t pos = name_.find("##");
    if(pos != std::string::npos)
    {
      return name_.substr(0, pos);
    }
    return name_;
  }

  inline const std::string & fullName() const
  {
    return name_;
  }

  bool required;

  inline const ::mc_rtc::imgui::Widget & parent() const
  {
    return parent_;
  }

  template<typename Derived, typename... Args>
  void update(Args &&... args)
  {
    if(locked_)
    {
      return;
    }
    static_cast<Derived *>(this)->update_(std::forward<Args>(args)...);
  }

  virtual void update(const mc_rtc::Configuration &) = 0;

protected:
  const ::mc_rtc::imgui::Widget & parent_;
  std::string name_;

  /** Unique id to further disambiguate labels */
  uint64_t id_ = 0;
  inline static uint64_t next_id_ = 0;
  bool locked_ = false;
};

struct ObjectWidget : public Widget
{
  /** The requiredOnly parameter is used in the OneOf widget to make this act as a container */
  ObjectWidget(const ::mc_rtc::imgui::Widget & parent,
               const std::string & name,
               ObjectWidget * parentForm,
               bool requiredOnly = false)
  : Widget(parent, name), parentForm_(parentForm), requiredOnly_(requiredOnly)
  {
  }

  WidgetPtr clone(ObjectWidget * parent) const override
  {
    return clone_object(parent);
  }

  ObjectWidgetPtr clone_object(ObjectWidget * parent) const
  {
    auto out = std::make_unique<ObjectWidget>(parent_, name_, parent, requiredOnly_);
    auto clone_widgets = [&out](const std::vector<WidgetPtr> & widgets_in, std::vector<WidgetPtr> & widgets_out)
    {
      for(const auto & w : widgets_in)
      {
        widgets_out.push_back(w->clone(out.get()));
      }
    };
    clone_widgets(requiredWidgets_, out->requiredWidgets_);
    clone_widgets(otherWidgets_, out->otherWidgets_);
    return out;
  }

  bool ready() override
  {
    return std::all_of(requiredWidgets_.begin(), requiredWidgets_.end(), [](const auto & w) { return w->ready(); });
  }

  void draw_() override
  {
    draw_(false);
  }

  void draw_(bool is_root)
  {
    auto drawWidgets = [this](std::vector<form::WidgetPtr> & widgets)
    {
      for(size_t i = 0; i < widgets.size(); ++i)
      {
        widgets[i]->draw();
        locked_ = locked_ || widgets[i]->locked();
        if(i + 1 != widgets.size())
        {
          IndentedSeparator();
        }
      }
    };
    if(!is_root)
    {
      IndentedSeparator();
      ImGui::Indent();
    }
    drawWidgets(requiredWidgets_);
    // FIXME Maybe always show if there is few optional elements?
    if(requiredWidgets_.size() == 0 || (otherWidgets_.size() && ImGui::CollapsingHeader(label("Optional").c_str())))
    {
      if(requiredWidgets_.size() != 0)
      {
        ImGui::Indent();
      }
      drawWidgets(otherWidgets_);
      if(requiredWidgets_.size() != 0)
      {
        ImGui::Unindent();
      }
    }
    if(!is_root)
    {
      ImGui::Unindent();
    }
  }

  void draw3D() override
  {
    for(auto & w : requiredWidgets_)
    {
      w->draw3D();
    }
    for(auto & w : otherWidgets_)
    {
      w->draw3D();
    }
  }

  bool trivial() const override
  {
    return false;
  }

  void collect(mc_rtc::Configuration & out_) override
  {
    mc_rtc::Configuration out = out_;
    if(parentForm_ != nullptr)
    {
      out = out.add(name_);
    }
    for(auto & w : requiredWidgets_)
    {
      w->collect(out);
      w->unlock();
    }
    for(auto & w : otherWidgets_)
    {
      if(w->ready())
      {
        w->collect(out);
        w->unlock();
      }
    }
    locked_ = false;
  }

  void update_(ObjectWidget * /*parent*/) {}

  void update(const mc_rtc::Configuration & config) override
  {
    if(locked_)
    {
      return;
    }
    std::map<std::string, mc_rtc::Configuration> values = config;
    for(auto & w : requiredWidgets_)
    {
      w->update(values.at(w->name()));
    }
    for(auto & w : otherWidgets_)
    {
      auto it = values.find(w->name());
      if(it != values.end())
      {
        w->update(it->second);
      }
    }
  }

  template<typename WidgetT, typename... Args>
  ObjectWidget * widget(const std::string & name, bool required, Args &&... args)
  {
    if(required || requiredOnly_)
    {
      return widget<WidgetT>(name, requiredWidgets_, std::forward<Args>(args)...);
    }
    else
    {
      return widget<WidgetT>(name, otherWidgets_, std::forward<Args>(args)...);
    }
  }

  inline ObjectWidget * parentForm() noexcept
  {
    return parentForm_;
  }

  using Widget::value;

  inline std::string value(const std::string & name) const
  {
    auto pred = [&](auto && w) { return w->fullName() == name; };
    auto it = std::find_if(requiredWidgets_.begin(), requiredWidgets_.end(), pred);
    if(it == requiredWidgets_.end())
    {
      it = std::find_if(otherWidgets_.begin(), otherWidgets_.end(), pred);
      if(it == otherWidgets_.end())
      {
        return "";
      }
    }
    return (*it)->value();
  }

  /** Returns all widgets in the object */
  inline const std::vector<form::WidgetPtr> & widgets() const noexcept
  {
    return requiredWidgets_;
  }

protected:
  ObjectWidget * parentForm_ = nullptr;
  bool requiredOnly_ = false;
  std::vector<form::WidgetPtr> requiredWidgets_;
  std::vector<form::WidgetPtr> otherWidgets_;

  template<typename WidgetT, typename... Args>
  ObjectWidget * widget(const std::string & name, std::vector<form::WidgetPtr> & widgets, Args &&... args);
};

struct ObjectArrayWidget : public Widget
{
  ObjectArrayWidget(const ::mc_rtc::imgui::Widget & parent,
                    const std::string & name,
                    bool required,
                    ObjectWidget * parentForm)
  : ObjectArrayWidget(parent, name, required, std::make_unique<ObjectWidget>(parent, name, parentForm))
  {
  }

  ObjectArrayWidget(const ::mc_rtc::imgui::Widget & parent,
                    const std::string & name,
                    bool required,
                    ObjectWidgetPtr primary)
  : Widget(parent, name), required_(required), primaryForm_(std::move(primary))
  {
  }

  WidgetPtr clone(ObjectWidget * parent) const override
  {
    return std::make_unique<ObjectArrayWidget>(parent_, name_, required_, primaryForm_->clone_object(parent));
  }

  bool ready() override
  {
    bool objects_ready = std::all_of(objects_.begin(), objects_.end(), [](const auto & w) { return w->ready(); });
    if(required_)
    {
      return objects_ready;
    }
    else
    {
      return objects_.size() && objects_ready;
    }
  }

  void draw_() override
  {
    IndentedSeparator();
    ImGui::Indent();
    std::vector<size_t> to_delete;
    for(size_t i = 0; i < objects_.size(); ++i)
    {
      parent_.client.enable_bold_font();
      ImGui::Text("[%zu]", i);
      ImGui::SameLine();
      if(ImGui::Button(label("-", i).c_str()))
      {
        locked_ = true;
        to_delete.push_back(i);
      }
      parent_.client.disable_bold_font();
      objects_[i]->draw_();
      locked_ = locked_ || objects_[i]->locked();
    }
    for(size_t i = to_delete.size(); i > 0; --i)
    {
      objects_.erase(objects_.begin() + to_delete[i - 1]);
    }
    IndentedSeparator();
    if(ImGui::Button(label("+").c_str()))
    {
      locked_ = true;
      objects_.push_back(primaryForm_->clone_object(nullptr));
    }
    ImGui::Unindent();
  }

  void draw3D() override
  {
    for(auto & o : objects_)
    {
      o->draw3D();
    }
  }

  bool trivial() const override
  {
    return false;
  }

  void collect(mc_rtc::Configuration & out_) override
  {
    mc_rtc::Configuration out = out_.array(name_);
    for(auto & o : objects_)
    {
      mc_rtc::Configuration c;
      o->collect(c);
      out.push(c);
    }
    objects_.clear();
    locked_ = false;
  }

  void update_(bool /*required*/, ObjectWidget * /*parent*/) {}

  void update(const mc_rtc::Configuration & data_) override
  {
    update(data_.operator std::vector<mc_rtc::Configuration>());
  }

  virtual void update(const std::vector<mc_rtc::Configuration> & data)
  {
    if(locked_)
    {
      return;
    }
    objects_.resize(data.size());
    for(size_t i = 0; i < objects_.size(); ++i)
    {
      auto & o = objects_[i];
      if(!o || !o->locked())
      {
        o = primaryForm_->clone_object(nullptr);
      }
      o->update(data[i]);
    }
  }

  inline ObjectWidget * primary() noexcept
  {
    return primaryForm_.get();
  }

protected:
  bool required_;
  ObjectWidgetPtr primaryForm_;
  std::vector<ObjectWidgetPtr> objects_;
};

struct GenericArrayWidget : public ObjectArrayWidget
{
  GenericArrayWidget(const ::mc_rtc::imgui::Widget & parent,
                     const std::string & name,
                     bool required,
                     ObjectWidget * parentForm,
                     const std::optional<std::vector<mc_rtc::Configuration>> & data)
  : GenericArrayWidget(parent, name, required, std::make_unique<ObjectWidget>(parent, name, parentForm, true), data)
  {
  }

  GenericArrayWidget(const ::mc_rtc::imgui::Widget & parent,
                     const std::string & name,
                     bool required,
                     ObjectWidgetPtr primary,
                     const std::optional<std::vector<mc_rtc::Configuration>> & data)
  : ObjectArrayWidget(parent, name, required, std::move(primary))
  {
    update_(required, primary.get(), data);
  }

  WidgetPtr clone(ObjectWidget * parent) const override
  {
    return std::make_unique<GenericArrayWidget>(parent_, name_, required_, primaryForm_->clone_object(parent), data_);
  }

  void collect(mc_rtc::Configuration & out_) override
  {
    mc_rtc::Configuration out = out_.array(name_);
    for(auto & o : objects_)
    {
      mc_rtc::Configuration c;
      o->collect(c);
      out.push(c(o->widgets()[0]->name()));
    }
    objects_.clear();
    locked_ = false;
  }

  void update_(bool /*required*/,
               ObjectWidget * /*parent*/,
               const std::optional<std::vector<mc_rtc::Configuration>> & data)
  {
    if(locked_)
    {
      return;
    }
    data_ = data;
    if(data)
    {
      update(*data);
    }
  }

  using ObjectArrayWidget::update;

  void update(const std::vector<mc_rtc::Configuration> & data) override
  {
    if(locked_)
    {
      return;
    }
    objects_.resize(data.size());
    for(size_t i = 0; i < objects_.size(); ++i)
    {
      auto & o = objects_[i];
      if(!o || o->widgets().size() == 0)
      {
        o = primaryForm_->clone_object(nullptr);
      }
      if(o->widgets().size())
      {
        o->widgets()[0]->update(data[i]);
      }
    }
  }

protected:
  std::optional<std::vector<mc_rtc::Configuration>> data_ = std::nullopt;
};

struct OneOfWidget : public Widget
{
  OneOfWidget(const ::mc_rtc::imgui::Widget & parent,
              const std::string & name,
              ObjectWidget * parentForm,
              const std::optional<std::pair<size_t, mc_rtc::Configuration>> & data)
  : OneOfWidget(parent, name, std::make_unique<ObjectWidget>(parent, name, parentForm, true), data)
  {
  }

  OneOfWidget(const ::mc_rtc::imgui::Widget & parent,
              const std::string & name,
              ObjectWidgetPtr primary,
              const std::optional<std::pair<size_t, mc_rtc::Configuration>> & data)
  : Widget(parent, name), container_(std::move(primary)), data_(data)
  {
  }

  WidgetPtr clone(ObjectWidget * parent) const override
  {
    return std::make_unique<OneOfWidget>(parent_, name_, container_->clone_object(parent), data_);
  }

  bool ready() override
  {
    return active_ && active_->ready();
  }

  void draw_() override
  {
    ImGui::SameLine();
    if(ImGui::BeginCombo(label("").c_str(), active_ ? active_->name().c_str() : ""))
    {
      if(ImGui::Selectable(label("", "selectable").c_str(), !active_))
      {
        locked_ = true;
        active_idx_ = std::numeric_limits<size_t>::max();
        active_.reset();
      }
      for(size_t i = 0; i < container_->widgets().size(); ++i)
      {
        auto & w = container_->widgets()[i];
        bool selected = active_ && active_->name() == w->name();
        if(ImGui::Selectable(label(w->name()).c_str(), selected))
        {
          if(!active_ || active_->name() != w->name())
          {
            locked_ = true;
            active_ = w->clone(nullptr);
            active_idx_ = i;
          }
        }
        if(active_ && active_->name() == w->name())
        {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    if(!active_)
    {
      return;
    }
    IndentedSeparator();
    ImGui::Spacing();
    ImGui::Indent();
    active_->draw_();
    locked_ = locked_ || active_->locked();
    ImGui::Unindent();
  }

  void draw3D() override
  {
    if(active_)
    {
      active_->draw3D();
    }
  }

  bool trivial() const override
  {
    return false;
  }

  void collect(mc_rtc::Configuration & out_) override
  {
    assert(active_);
    auto out = out_.array(name_, 2);
    out.push(active_idx_);
    Configuration object_out;
    active_->collect(object_out);
    out.push(object_out(active_->name()));
    locked_ = false;
    active_idx_ = std::numeric_limits<size_t>::max();
    active_.reset();
  }

  void update_(ObjectWidget * /*parent*/, const std::optional<std::pair<size_t, mc_rtc::Configuration>> & data)
  {
    if(data)
    {
      update(*data);
    }
  }

  void update(const std::pair<size_t, mc_rtc::Configuration> & data)
  {
    if(locked_)
    {
      return;
    }
    data_ = data;
    if(data.first >= container_->widgets().size())
    {
      return;
    }
    if(data.first != active_idx_)
    {
      active_idx_ = data.first;
      active_ = container_->widgets()[active_idx_]->clone(nullptr);
    }
    active_->update(data.second);
  }

  void update(const mc_rtc::Configuration & data_) override
  {
    update(data_.operator std::pair<size_t, mc_rtc::Configuration>());
  }

  inline ObjectWidget * container() noexcept
  {
    return container_.get();
  }

private:
  ObjectWidgetPtr container_;
  WidgetPtr active_;
  size_t active_idx_ = std::numeric_limits<size_t>::max();
  std::optional<std::pair<size_t, mc_rtc::Configuration>> data_;
};

template<typename DataT>
struct SimpleInput : public Widget
{
  SimpleInput(const ::mc_rtc::imgui::Widget & parent, const std::string & name) : Widget(parent, name) {}

  SimpleInput(const ::mc_rtc::imgui::Widget & parent,
              const std::string & name,
              const std::optional<DataT> & value,
              const std::optional<DataT> & temp = std::nullopt)
  : Widget(parent, name), value_(value)
  {
    if(temp.has_value())
    {
      temp_ = temp.value();
    }
    else if(value_.has_value())
    {
      temp_ = value.value();
    }
    else
    {
      if constexpr(std::is_arithmetic_v<DataT>)
      {
        temp_ = 0;
      }
      else
      {
        if constexpr(std::is_same_v<DataT, sva::PTransformd>)
        {
          temp_ = sva::PTransformd::Identity();
        }
        else if constexpr(std::is_same_v<DataT, Eigen::Vector3d>)
        {
          temp_ = Eigen::Vector3d::Zero();
        }
        else
        {
          temp_ = {};
        }
      }
    }
  }

  ~SimpleInput() override = default;

  bool ready() override
  {
    if constexpr(std::is_same_v<DataT, std::string> || std::is_same_v<DataT, Eigen::VectorXd>)
    {
      return value_.has_value() && value_.value().size() != 0;
    }
    else
    {
      return value_.has_value();
    }
  }

  void collect(mc_rtc::Configuration & out) override
  {
    if(ready())
    {
      out.add(name(), value_.value());
    }
    else
    {
      out.add(name(), temp_);
    }
  }

  std::string value() override
  {
    if constexpr(std::is_same_v<DataT, std::string>)
    {
      return value_.has_value() ? value_.value() : "";
    }
    else
    {
      return fmt::format("{}", value_.has_value() ? value_.value() : temp_);
    }
  }

  void update_(const std::optional<DataT> & value)
  {
    value_ = value;
    if(value_.has_value())
    {
      temp_ = value_.value();
    }
  }

  void update(const mc_rtc::Configuration & data) override
  {
    if(locked_)
    {
      return;
    }
    update_(data.operator DataT());
  }

protected:
  std::optional<DataT> value_;
  DataT temp_;
};

struct Checkbox : public SimpleInput<bool>
{
  using SimpleInput::SimpleInput;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<Checkbox>(parent_, name_, value_);
  }

  inline void draw_() override
  {
    ImGui::SameLine();
    if(ImGui::Checkbox(label("").c_str(), &temp_))
    {
      value_ = temp_;
      locked_ = true;
    }
  }
};

struct IntegerInput : public SimpleInput<int>
{
  using SimpleInput::SimpleInput;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<IntegerInput>(parent_, name_, value_);
  }

  inline void draw_() override
  {
    ImGui::SameLine();
    if(ImGui::InputInt(label("").c_str(), &temp_, 0, 0))
    {
      value_ = temp_;
      locked_ = true;
    }
  }
};

struct NumberInput : public SimpleInput<double>
{
  using SimpleInput::SimpleInput;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<NumberInput>(parent_, name_, value_);
  }

  inline void draw_() override
  {
    ImGui::SameLine();
    if(ImGui::InputDouble(label("").c_str(), &temp_))
    {
      value_ = temp_;
      locked_ = true;
    }
  }
};

struct StringInput : public SimpleInput<std::string>
{
  using SimpleInput::SimpleInput;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<StringInput>(parent_, name_, value_);
  }

  inline void draw_() override
  {
    ImGui::SameLine();
    const auto & value = value_.has_value() ? value_.value() : "";
    if(buffer_.size() < std::max<size_t>(value.size() + 1, 256))
    {
      buffer_.resize(std::max<size_t>(value.size() + 1, 256));
    }
    std::memcpy(buffer_.data(), value.data(), value.size());
    buffer_[value.size()] = 0;
    if(ImGui::InputText(label("").c_str(), buffer_.data(), buffer_.size()))
    {
      value_ = {buffer_.data(), strnlen(buffer_.data(), buffer_.size())};
      locked_ = true;
    }
  }

private:
  std::vector<char> buffer_;
};

struct ArrayInput : public SimpleInput<Eigen::VectorXd>
{
  ArrayInput(const ::mc_rtc::imgui::Widget & parent,
             const std::string & name,
             const std::optional<Eigen::VectorXd> & default_,
             bool fixed_size);

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<ArrayInput>(parent_, name_, value_, fixed_);
  }

  void draw_() override;

  inline void update_(const std::optional<Eigen::VectorXd> & value, bool fixed)
  {
    SimpleInput<Eigen::VectorXd>::update_(value);
    fixed_ = fixed;
  }

private:
  bool fixed_;
};

template<typename T>
static inline sva::PTransformd value_or(const std::optional<T> & data)
{
  if constexpr(std::is_same_v<T, Eigen::Vector3d>)
  {
    return {data.value_or(Eigen::Vector3d::Zero())};
  }
  else
  {
    return data.value_or(sva::PTransformd::Identity());
  }
}

template<typename DataT, ::mc_rtc::imgui::ControlAxis axis>
struct InteractiveMarkerInput : public SimpleInput<DataT>
{
  InteractiveMarkerInput(const ::mc_rtc::imgui::Widget & parent,
                         const std::string & name,
                         const std::optional<DataT> & default_,
                         bool interactive)
  : SimpleInput<DataT>(parent, name, default_), marker_(parent.client.make_marker(value_or(default_), axis)),
    interactive_(interactive)
  {
  }

  inline void draw_() override
  {
    if(!interactive_)
    {
      return;
    }
    ImGui::SameLine();
    if(ImGui::Button(this->label(visible_ ? "Hide" : "Show").c_str()))
    {
      visible_ = !visible_;
    }
  }

  inline void draw_translation_input(Eigen::Vector3d & data)
  {
    ImGui::BeginTable(this->label("", "table_translation").c_str(), 3, ImGuiTableFlags_SizingStretchProp);
    static std::array<const char *, 3> labels = {"x", "y", "z"};
    for(size_t i = 0; i < 3; ++i)
    {
      ImGui::TableNextColumn();
      ImGui::Text("%s", labels[i]);
    }
    for(size_t i = 0; i < 3; ++i)
    {
      ImGui::TableNextColumn();
      if(ImGui::InputDouble(this->label("", fmt::format("table_translation_{}", i)).c_str(), &data(i)))
      {
        if constexpr(std::is_same_v<DataT, Eigen::Vector3d>)
        {
          this->value_ = data;
          if(marker_)
          {
            marker_->pose({data});
          }
        }
        else
        {
          this->value_.value().translation() = data;
          if(marker_)
          {
            marker_->pose(data);
          }
        }
        this->locked_ = true;
      }
    }
    ImGui::EndTable();
  }

  inline void draw_quaternion_input(Eigen::Matrix3d & rot)
  {
    Eigen::Quaterniond quat(rot);
    ImGui::BeginTable(this->label("", "table_quaternion").c_str(), 4, ImGuiTableFlags_SizingStretchProp);
    static std::array<const char *, 4> labels = {"w", "x", "y", "z"};
    for(size_t i = 0; i < 4; ++i)
    {
      ImGui::TableNextColumn();
      ImGui::Text("%s", labels[i]);
    }
    auto get_ptr = [&](size_t i)
    {
      if(i == 0)
      {
        return &quat.w();
      }
      else if(i == 1)
      {
        return &quat.x();
      }
      else if(i == 2)
      {
        return &quat.y();
      }
      else
      {
        return &quat.z();
      }
    };
    for(size_t i = 0; i < 4; ++i)
    {
      ImGui::TableNextColumn();
      if(ImGui::InputDouble(this->label("", fmt::format("table_quaternion_{}", i)).c_str(), get_ptr(i)))
      {
        this->temp_.rotation() = quat.toRotationMatrix();
        this->value_ = this->temp_;
        if(marker_)
        {
          marker_->pose(this->temp_);
        }
        this->locked_ = true;
      }
    }
    ImGui::EndTable();
  }

  inline void draw3D() override
  {
    if(!interactive_)
    {
      return;
    }
    if(visible_ && marker_)
    {
      if(marker_->draw())
      {
        this->locked_ = true;
        if constexpr(std::is_same_v<DataT, Eigen::Vector3d>)
        {
          this->temp_ = marker_->pose().translation();
        }
        else
        {
          static_assert(std::is_same_v<DataT, sva::PTransformd>);
          this->temp_ = marker_->pose();
        }
        this->value_ = this->temp_;
      }
    }
  }

  void update_(const std::optional<DataT> & value, bool interactive)
  {
    SimpleInput<DataT>::update_(value);
    interactive_ = interactive;
    marker_->pose(this->temp_);
  }

protected:
  ::mc_rtc::imgui::InteractiveMarkerPtr marker_;
  bool interactive_;
  bool visible_ = false;
};

struct Point3DInput : public InteractiveMarkerInput<Eigen::Vector3d, ::mc_rtc::imgui::ControlAxis::TRANSLATION>
{
  using Base = InteractiveMarkerInput<Eigen::Vector3d, ::mc_rtc::imgui::ControlAxis::TRANSLATION>;
  using Base::Base;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<Point3DInput>(parent_, name_, value_, interactive_);
  }

  inline void draw_() override
  {
    Base::draw_();
    draw_translation_input(temp_);
  }
};

struct RotationInput : public InteractiveMarkerInput<sva::PTransformd, ::mc_rtc::imgui::ControlAxis::ROTATION>
{
  using Base = InteractiveMarkerInput<sva::PTransformd, ::mc_rtc::imgui::ControlAxis::ROTATION>;
  using Base::Base;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<RotationInput>(parent_, name_, value_, interactive_);
  }

  inline void draw_() override
  {
    Base::draw_();
    draw_quaternion_input(temp_.rotation());
  }

  void collect(mc_rtc::Configuration & out) override
  {
    Eigen::Quaterniond quat(ready() ? value_.value().rotation() : temp_.rotation());
    out.add(name(), quat);
  }
};

struct TransformInput : public InteractiveMarkerInput<sva::PTransformd, ::mc_rtc::imgui::ControlAxis::ALL>
{
  using Base = InteractiveMarkerInput<sva::PTransformd, ::mc_rtc::imgui::ControlAxis::ALL>;
  using Base::Base;

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<TransformInput>(parent_, name_, value_, interactive_);
  }

  inline void draw_() override
  {
    Base::draw_();
    draw_translation_input(temp_.translation());
    draw_quaternion_input(temp_.rotation());
  }
};

struct ComboInput : public SimpleInput<std::string>
{
  ComboInput(const ::mc_rtc::imgui::Widget & parent,
             const std::string & name,
             const std::vector<std::string> & values,
             bool send_index,
             int user_default = -1);

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<ComboInput>(parent_, name_, values_, send_index_, idx_);
  }

  void draw_() override;

  inline void collect(mc_rtc::Configuration & out) override
  {
    assert(ready());
    if(send_index_)
    {
      out.add(name(), static_cast<unsigned int>(idx_));
    }
    else
    {
      out.add(name(), value_.value());
    }
  }

  void update_(const std::vector<std::string> & values, bool send_index, int user_default = -1);

  void update(const mc_rtc::Configuration & data) override;

protected:
  std::vector<std::string> values_;
  size_t idx_;
  bool send_index_;

  void draw(const char * label);
};

struct DataComboInput : public ComboInput
{
  DataComboInput(const ::mc_rtc::imgui::Widget & parent,
                 const std::string & name,
                 const std::vector<std::string> & ref,
                 bool send_index);

  WidgetPtr clone(ObjectWidget *) const override
  {
    return std::make_unique<DataComboInput>(parent_, name_, ref_, send_index_);
  }

  void draw_() override;

protected:
  std::vector<std::string> ref_;
};

template<typename WidgetT, typename... Args>
ObjectWidget * ObjectWidget::widget(const std::string & name, std::vector<form::WidgetPtr> & widgets, Args &&... args)
{
  auto it = std::find_if(widgets.begin(), widgets.end(), [&](const auto & w) { return w->fullName() == name; });
  if(it == widgets.end())
  {
    widgets.push_back(std::make_unique<WidgetT>(parent_, name, std::forward<Args>(args)...));
    it = widgets.end() - 1;
  }
  else
  {
    (*it)->template update<WidgetT>(std::forward<Args>(args)...);
  }
  if constexpr(std::is_same_v<WidgetT, ObjectWidget>)
  {
    return static_cast<ObjectWidget *>(it->get());
  }
  else if constexpr(std::is_same_v<WidgetT, ObjectArrayWidget>)
  {
    return static_cast<ObjectArrayWidget *>(it->get())->primary();
  }
  else if constexpr(std::is_same_v<WidgetT, GenericArrayWidget>)
  {
    return static_cast<GenericArrayWidget *>(it->get())->primary();
  }
  else if constexpr(std::is_same_v<WidgetT, OneOfWidget>)
  {
    return static_cast<OneOfWidget *>(it->get())->container();
  }
  else
  {
    return this;
  }
}

} // namespace form

} // namespace mc_rtc::imgui
