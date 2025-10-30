#pragma once

#include <utility>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light.h"

namespace esphome {
namespace synchronized_light {

class SynchronizedLinkedLight {
 public:
  SynchronizedLinkedLight(light::AddressableLightState *src) {
        src_ = static_cast<light::AddressableLight *>(src->get_output());
      }

  light::AddressableLight *get_src() const { return this->src_; }
  
  void setup_map(std::vector<std::tuple<int32_t, int32_t>> lights_map) {
    lights_map_ = std::move(lights_map);
  }

  std::tuple<int32_t, int32_t>& map_primary_light(int32_t index) {
    return lights_map_[index];
  }

 protected:
  light::AddressableLight *src_ = nullptr;  
  std::vector<std::tuple<int32_t, int32_t>> lights_map_;
};

class SynchronizedPrimaryLight {
 public:
  SynchronizedPrimaryLight(light::AddressableLightState *src) {
        src_ = static_cast<light::AddressableLight *>(src->get_output());
      }

  light::AddressableLight *get_src() const { return this->src_; }
  
 protected:
  light::AddressableLight *src_ = nullptr;
};

class SynchronizedLightOutput : public light::AddressableLight {
 public:
  explicit SynchronizedLightOutput(
    SynchronizedPrimaryLight *primary_light,
    std::vector<SynchronizedLinkedLight> linked_lights
  ) {
    primary_light_ = primary_light;
    linked_lights_ = linked_lights;
  }

  int32_t size() const override {
    return this->primary_light_->get_src()->size();
  }

  void clear_effect_data() override {
    this->primary_light_->get_src()->clear_effect_data();
    for (auto &linked_light : this->linked_lights_) {
      linked_light.get_src()->clear_effect_data();
    }
  }

  void setup() override {
    int32_t primary_size = this->primary_light_->get_src()->size();
    for (auto &linked_light : this->linked_lights_) {
      float ratio = linked_light.get_src()->size() / (float)primary_size;
      float cursor = 0;

      std::vector<std::tuple<int32_t, int32_t>> lights_map;
      lights_map.reserve(primary_size);

      for (auto primary_led_index = 0; primary_led_index < primary_size; primary_led_index++) {
        int32_t begin = (int32_t)cursor;
        cursor += ratio;
        auto end = (int32_t)(cursor);
        
        // the last LED of the source should always turns on the last LED of the linked_light
        if (primary_led_index == primary_size - 1) {
          end = linked_light.get_src()->size() - 1;
        }
        
        lights_map.push_back(std::make_tuple(begin, end));
      }

      linked_light.setup_map(lights_map);
    }
  }

  light::LightTraits get_traits() override { return this->primary_light_->get_src()->get_traits(); }

  void write_to_target(SynchronizedLinkedLight *target, int32_t primary_index, Color color) {
    auto tuple = target->map_primary_light(primary_index);

    int32_t target_begin = std::get<0>(tuple);
    int32_t target_end = std::get<1>(tuple);

    if (target_begin == target_end) {      
      auto target_light = target->get_src();
      auto target_state = (*target_light)[target_begin];
      target_state.set(color);
    } else {
      auto target_light = target->get_src();
      auto target_range = target_light->range(target_begin, target_end);
      target_range.set(color);
    }
  }

  void write_state(light::LightState *state) override {
    auto* primary_src = this->primary_light_->get_src();
    int32_t primary_size = primary_src->size();
    primary_src->schedule_show();
    
    std::vector<Color> primary_colors;
    primary_colors.reserve(primary_size);
    for (int32_t i = 0; i < primary_size; i++) {
      primary_colors.push_back((*primary_src)[i].get());
    }

    for (auto &linked_light : this->linked_lights_) {    
      for (auto i = 0; i < primary_size; i++) {                
        this->write_to_target(&linked_light, i, primary_colors[i]);
      }    
      linked_light.get_src()->schedule_show();
    }

    this->mark_shown_();
  }

 protected:
  light::ESPColorView get_view_internal(int32_t index) const override {
    auto light = this->primary_light_->get_src();
    auto view = (*light)[index];
    view.raw_set_color_correction(&this->correction_);
    return view;
  }

  std::vector<SynchronizedLinkedLight> linked_lights_;
  SynchronizedPrimaryLight *primary_light_;
};

}  // namespace partition
}  // namespace esphome
