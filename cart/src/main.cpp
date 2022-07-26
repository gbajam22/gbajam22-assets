#include "bn_core.h"
#include "bn_math.h"
#include "bn_keypad.h"
#include "bn_display.h"
#include "bn_affine_bg_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_affine_bg_pa_register_hbe_ptr.h"
#include "bn_affine_bg_pc_register_hbe_ptr.h"
#include "bn_affine_bg_dx_register_hbe_ptr.h"
#include "bn_affine_bg_dy_register_hbe_ptr.h"

#include "bn_blending_transparency_attributes.h"
#include "bn_blending_transparency_attributes_hbe_ptr.h"

#include "bn_affine_bg_items_map.h"
#include "bn_regular_bg_items_black.h"
#include "bn_sprite_items_logo.h"
#include "bn_sprite_items_start.h"
#include "bn_sprite_items_extra.h"

#include "fixed_8x8_sprite_font.h"

namespace
{
    struct camera
    {
        bn::fixed x = 32;
        bn::fixed y = 100;
        bn::fixed z = 0;
        int phi = 0;
        int cos = 0;
        int sin =20;
        unsigned int t = 0;
    };
    

    void update_camera(camera& camera)
    {
        bn::fixed dir_x = 0;
        bn::fixed dir_z = 0;

        camera.t += 1;

        dir_z -= bn::fixed::from_data(32);

        camera.cos = bn::lut_cos(camera.phi).data() >> 4;
        camera.sin = bn::lut_sin(camera.phi).data() >> 4;
        camera.x += (dir_x * camera.cos) - (dir_z * camera.sin) + bn::lut_sin(camera.t % 2048);
        camera.z += (dir_x * camera.sin) + (dir_z * camera.cos);
    }

    void update_hbe_values(const camera& camera, int16_t* pa_values, int16_t* pc_values, int* dx_values,
                           int* dy_values)
    {
        int camera_x = camera.x.data();
        int camera_y = camera.y.data() >> 4;
        int camera_z = camera.z.data();
        int camera_cos = camera.cos;
        int camera_sin = camera.sin;
        int y_shift = 160;

        for(int index = bn::display::height()/2; index < bn::display::height(); ++index)
        {
            int offset = ((index - 80) * 160)/80;

            int reciprocal = bn::reciprocal_lut[offset].data() >> 4;
            int lam = camera_y * reciprocal >> 12;
            int lcf = lam * camera_cos >> 8;
            int lsf = lam * camera_sin >> 8;

            pa_values[index] = int16_t(lcf >> 4);
            pc_values[index] = int16_t(lsf >> 4);

            int lxr = (bn::display::width() / 2) * lcf;
            int lyr = y_shift * lsf;
            dx_values[index] = (camera_x - lxr + lyr) >> 4;

            lxr = (bn::display::width() / 2) * lsf;
            lyr = y_shift * lcf;
            dy_values[index] = (camera_z - lxr - lyr) >> 4;
        }

        for(int index = 0; index < bn::display::height()/2; ++index)
        {
            int offset = 160 - ((index) * 160)/80;

            int reciprocal = bn::reciprocal_lut[offset].data() >> 4;
            int lam = camera_y * reciprocal >> 12;
            int lcf = lam * camera_cos >> 8;
            int lsf = lam * camera_sin >> 8;

            pa_values[index] = int16_t(lcf >> 4);
            pc_values[index] = int16_t(lsf >> 4);

            int lxr = (bn::display::width() / 2) * lcf;
            int lyr = y_shift * lsf;
            dx_values[index] = (camera_x - lxr + lyr) >> 4;

            lxr = (bn::display::width() / 2) * lsf;
            lyr = y_shift * lcf;
            dy_values[index] = (camera_z - lxr - lyr) >> 4;
        }
    }

    void setup_transparency_attributes(bn::blending_transparency_attributes* transparency_attributes)
    {
        constexpr int half_height = bn::display::height() / 2;
        constexpr int black_range = 16;

        int index = 0;

        for(; index < half_height - black_range; ++index)
        {
            bn::fixed transparency_alpha = bn::fixed(index) / (half_height - black_range);
            transparency_attributes[index].set_transparency_alpha(transparency_alpha);
            transparency_attributes[bn::display::height() - index - 1].set_transparency_alpha(transparency_alpha);
        }

        for(; index < half_height + black_range; ++index)
        {
            transparency_attributes[index].set_transparency_alpha(1);
        }
    }
}

int main()
{
    bn::core::init();

    bn::affine_bg_ptr bg = bn::affine_bg_items::map.create_bg(-376, -336);


    // distance fade
    bn::regular_bg_ptr black = bn::regular_bg_items::black.create_bg(0, 0);
    black.set_blending_enabled(true);

    bn::array<bn::blending_transparency_attributes, bn::display::height()> transparency_attributes;
    setup_transparency_attributes(transparency_attributes._data);

    bn::blending_transparency_attributes_hbe_ptr transparency_attributes_hbe =
            bn::blending_transparency_attributes_hbe_ptr::create(transparency_attributes);

    const bn::sprite_item& logo = bn::sprite_items::logo;

    bn::vector<bn::sprite_ptr, 6> logo_parts;
    logo_parts.push_back(logo.create_sprite(-64,-32-16,0));
    logo_parts.push_back(logo.create_sprite(0, -32-16,1));
    logo_parts.push_back(logo.create_sprite(64, -32-16,2));
    logo_parts.push_back(logo.create_sprite(-64, 32-16,3));
    logo_parts.push_back(logo.create_sprite(0, 32-16,4));
    logo_parts.push_back(logo.create_sprite(64, 32-16,5));

    // extra info, itch link
    bn::vector<bn::sprite_ptr, 5> extra;

    bn::sprite_ptr start = bn::sprite_items::start.create_sprite(-1, 56, 0);

    int16_t pa_values[bn::display::height()];
    bn::affine_bg_pa_register_hbe_ptr pa_hbe = bn::affine_bg_pa_register_hbe_ptr::create(bg, pa_values);

    int16_t pc_values[bn::display::height()];
    bn::affine_bg_pc_register_hbe_ptr pc_hbe = bn::affine_bg_pc_register_hbe_ptr::create(bg, pc_values);

    int dx_values[bn::display::height()];
    bn::affine_bg_dx_register_hbe_ptr dx_hbe = bn::affine_bg_dx_register_hbe_ptr::create(bg, dx_values);

    int dy_values[bn::display::height()];
    bn::affine_bg_dy_register_hbe_ptr dy_hbe = bn::affine_bg_dy_register_hbe_ptr::create(bg, dy_values);

    camera camera;

    // for start button movement
    bn::fixed y_pos = 0;
    int d = 1;

    bool showExtra = false;

    while(true)
    {
        // switch between logo and itch link on any key
        if(bn::keypad::any_pressed()){
            if(showExtra){
                extra.clear();
                start.set_visible(true);
                logo_parts.push_back(logo.create_sprite(-64,-32-16,0));
                logo_parts.push_back(logo.create_sprite(0, -32-16,1));
                logo_parts.push_back(logo.create_sprite(64, -32-16,2));
                logo_parts.push_back(logo.create_sprite(-64, 32-16,3));
                logo_parts.push_back(logo.create_sprite(0, 32-16,4));
                logo_parts.push_back(logo.create_sprite(64, 32-16,5));
            } else {
                start.set_item(bn::sprite_items::start, 1);
                bn::core::update();
                start.set_visible(false);
                logo_parts.clear();
                extra.push_back(bn::sprite_items::extra.create_sprite(-64,0,0));
                extra.push_back(bn::sprite_items::extra.create_sprite(-32,0,1));
                extra.push_back(bn::sprite_items::extra.create_sprite(0,0,2));
                extra.push_back(bn::sprite_items::extra.create_sprite(32,0,3));
                extra.push_back(bn::sprite_items::extra.create_sprite(64,0,4));
            }
            showExtra = !showExtra;
        }

        // move start button
        start.set_y(56 + y_pos/5);
        y_pos+=d * bn::fixed(0.5);
        if(y_pos > 10 || y_pos < -10){
            d = d * - 1;
        }

        update_camera(camera);
        update_hbe_values(camera, pa_values, pc_values, dx_values, dy_values);
        pa_hbe.reload_values_ref();
        pc_hbe.reload_values_ref();
        dx_hbe.reload_values_ref();
        dy_hbe.reload_values_ref();
        bn::core::update();

    }
}
