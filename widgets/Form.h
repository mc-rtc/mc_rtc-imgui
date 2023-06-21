#pragma once

#include "Widget.h"

#include "IndentedSeparator.h"
#include "form/widgets.h"

namespace mc_rtc::imgui
{

// FIXME Does not update if the form is changed between two calls
struct Form : public Widget
{
  Form(Client & client, const ElementId & id)
  : Widget(client, id), object_(std::make_unique<form::ObjectWidget>(*this, "", nullptr))
  {
  }

  template<typename WidgetT, typename... Args>
  form::ObjectWidget * widget(const std::string & name, bool required, Args &&... args)
  {
    auto out = object_->widget<WidgetT>(name, required, std::forward<Args>(args)...);
    if constexpr(std::is_same_v<WidgetT, form::ObjectWidget>)
    {
      return out;
    }
    else
    {
      return object_.get();
    }
  }

  inline std::string value(const std::string & name) const
  {
    return object_->value(name);
  }

  void draw2D() override
  {
    object_->draw_(true);
    if(ImGui::Button(label(id.name).c_str()))
    {
      if(!object_->ready())
      {
        // FIXME SHOW A POPUP WITH ALL REQUESTED FIELDS MISSING
        mc_rtc::log::critical("Form not ready");
        return;
      }
      mc_rtc::Configuration data;
      object_->collect(data);
      client.send_request(id, data);
    }
  }

  void draw3D() override
  {
    object_->draw3D();
  }

  inline form::ObjectWidget * parentForm() noexcept
  {
    return object_.get();
  }

protected:
  form::ObjectWidgetPtr object_;
};

} // namespace mc_rtc::imgui
