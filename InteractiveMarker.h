#pragma once

#include <SpaceVecAlg/SpaceVecAlg>

#include <memory>
#include <type_traits>

namespace mc_rtc::imgui
{

enum class ControlAxis
{
  NONE = 0,
  TX = (1u << 0),
  TY = (1u << 1),
  TZ = (1u << 2),
  RX = (1u << 3),
  RY = (1u << 4),
  RZ = (1u << 5),
  TRANSLATION = TX | TY | TZ,
  ROTATION = RX | RY | RZ,
  XYTHETA = TX | TY | RZ,
  XYZTHETA = TX | TY | TZ | RZ,
  ALL = TRANSLATION | ROTATION
};

inline ControlAxis operator|(ControlAxis lhs, ControlAxis rhs)
{
  using enum_t = std::underlying_type_t<ControlAxis>;
  return static_cast<ControlAxis>(static_cast<enum_t>(lhs) | static_cast<enum_t>(rhs));
}

inline ControlAxis operator&(ControlAxis lhs, ControlAxis rhs)
{
  using enum_t = std::underlying_type_t<ControlAxis>;
  return static_cast<ControlAxis>(static_cast<enum_t>(lhs) & static_cast<enum_t>(rhs));
}

/** Abstract interface for an interactive marker in the 3D GUI */
struct InteractiveMarker
{
  /** Create an interactive marker at the given position with the provided axis controllable */
  inline InteractiveMarker(const sva::PTransformd & pose, ControlAxis mask = ControlAxis::NONE)
  : pose_(pose), mask_(mask)
  {
  }

  virtual ~InteractiveMarker() = 0;

  inline const sva::PTransformd & pose() const noexcept
  {
    return pose_;
  }

  /** Change the interaction mask of the marker */
  virtual void mask(ControlAxis mask) = 0;

  /** Change the position of the marker */
  virtual void pose(const sva::PTransformd & pose) = 0;

  /** Draw the marker
   *
   * \returns True if the user moved the marker in which case \ref pose() has the new position
   */
  virtual bool draw() = 0;

protected:
  sva::PTransformd pose_;
  ControlAxis mask_;
};

using InteractiveMarkerPtr = std::unique_ptr<InteractiveMarker>;

} // namespace mc_rtc::imgui
