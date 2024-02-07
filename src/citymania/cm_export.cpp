#include "../stdafx.h"

#include "cm_export.hpp"

#include "../cargotype.h"
#include "../house.h"  // NUM_HOUSES and HZ_* for _town_land.h
#include "../gfx_func.h"
#include "../gfx_type.h"
#include "../engine_base.h"
#include "../palette_func.h"  // _colour_gradient
#include "../spritecache.h"
#include "../strings_func.h"
#include "../strings_type.h"
#include "../table/palettes.h"
#include "../table/sprites.h"
#include "../table/strings.h"  // for town_land.h
#include "../table/train_sprites.h"
#include "../table/town_land.h"  // _town_draw_tile_data

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "../safeguards.h"

namespace citymania {

extern SpriteID (*GetDefaultTrainSprite)(uint8_t, Direction);  // train_cmd.cpp

namespace data_export {

class JsonWriter {
protected:
    int i = 0;
    bool no_comma = true;
    char buffer[128];

public:
    std::ofstream f;

    JsonWriter(const std::string &fname) {
        f.open(fname.c_str());
        f << "OPENTTD = {";
        no_comma = true;
    }

    ~JsonWriter() {
        this->ident(false);
        f << "}" << std::endl;
        f.close();
    }

    void ident(bool comma=true) {
        if (comma && !no_comma) f << ",";
        no_comma = false;
        f << std::endl;
        for(int j = 0; j < i; j++) f << "  ";
    }

    void key(const char *k) {
        const char *kn;
        for (kn = k + strlen(k); kn >= k && *kn != '>' && *kn != '.'; kn--);
        kn++;
        this->ident();
        f << "\"" << kn << "\": ";
    }

    void value(bool val) {
        f << (val ? "true" : "false");
    }

    void value(unsigned int val) {
        f << val;
    }

    void value(uint64_t val) {
        f << val;
    }

    void value(Money val) {
        f << val;
    }

    void value(int val) {
        f << val;
    }

    void value(const char *v) {
        f << "\"" << v << "\"";
    }

    void value(const std::string &s) {
        f << "\"" << s << "\"";
    }

    template<typename T>
    void kv(const char *k, T v) {
        key(k);
        value(v);
    }

    void ks(const char *k, StringID s) {
        key(k);
        value(GetString(s));
    }

    void begin_dict_with_key(const char *k) {
        key(k);
        f << "{";
        no_comma = true;
        i++;
    }

    void begin_dict() {
        this->ident();
        f << "{";
        no_comma = true;
        i++;
    }

    void end_dict() {
        i--;
        this->ident(false);
        f << "}";
    }

    void begin_list_with_key(const char *k) {
        key(k);
        f << "[";
        no_comma = true;
        i++;
    }

    void end_list() {
        i--;
        this->ident(false);
        f << "]";
    }
};

#define JKV(j, field) j.kv(#field, field)

void WriteHouseSpecInfo(JsonWriter &j) {
    j.begin_list_with_key("house_specs");
    for (uint i = 0; i < NUM_HOUSES; i++) {
        const HouseSpec *hs = HouseSpec::Get(i);
        j.begin_dict();
        j.kv("min_year", hs->min_year.base());
        j.kv("max_year", hs->max_year.base());
        j.kv("population", hs->population);
        j.kv("removal_cost", hs->removal_cost);
        j.kv("name", hs->building_name);
        j.kv("mail_generation", hs->mail_generation);
        j.kv("flags", hs->building_flags);
        j.kv("availability", hs->building_availability);
        j.kv("enabled", hs->enabled);
        j.end_dict();
    }
    j.end_list();
    j.begin_list_with_key("house_draw_tile_data");
    for (auto &d : _town_draw_tile_data) {
        j.begin_dict();
        j.kv("ground_sprite", d.ground.sprite);
        j.kv("ground_pal", d.ground.pal);
        j.kv("building_sprite", d.building.sprite);
        j.kv("building_pal", d.building.pal);
        j.kv("subtile_x", d.subtile_x);
        j.kv("subtile_y", d.subtile_y);
        j.kv("width", d.width);
        j.kv("height", d.height);
        j.kv("dz", d.dz);
        j.kv("draw_proc", d.draw_proc);
        j.end_dict();
    }
    j.end_list();
}

void WriteCargoSpecInfo(JsonWriter &j) {
    j.begin_list_with_key("cargo_specs");
    const CargoSpec *cs;
    char buffer[128];
    char cargo_label[16];
    bool first = true;
    SetDParam(0, 123);
    for (const CargoSpec *cs : CargoSpec::Iterate()) {
        j.begin_dict();
        JKV(j, cs->initial_payment);
        j.kv("id", cs->bitnum);
        j.kv("transit_periods_1", cs->transit_periods[0]);
        j.kv("transit_periods_2", cs->transit_periods[1]);
        JKV(j, cs->weight);
        JKV(j, cs->multiplier);
        JKV(j, cs->is_freight);
        JKV(j, cs->legend_colour);
        JKV(j, cs->rating_colour);
        JKV(j, cs->sprite);
        j.ks("name", cs->name);
        j.ks("name_single", cs->name_single);
        j.ks("units_volume", cs->units_volume);
        j.ks("quantifier", cs->quantifier);
        j.ks("abbrev", cs->abbrev);

        auto label = cs->label.base();
        for (uint i = 0; i < sizeof(cs->label); i++) {
            cargo_label[i] = GB(label, (uint8_t)(sizeof(label) - i - 1) * 8, 8);
        }
        cargo_label[sizeof(label)] = '\0';
        JKV(j, label);
        j.kv("label_str", cargo_label);
        j.end_dict();
    }
    j.end_list();
}

// extern const Palette _palette;
void WritePaletteInfo(JsonWriter &j) {
    j.begin_list_with_key("palette");
    for (uint i = 0; i < 256; i++) {
        j.begin_dict();
        auto &c = _palette.palette[i];
        JKV(j, c.r);
        JKV(j, c.g);
        JKV(j, c.b);
        std::ostringstream hex;
        hex << "#" << std::hex << std::setfill('0');
        hex << std::setw(2) << (int)c.r;
        hex << std::setw(2) << (int)c.g;
        hex << std::setw(2) << (int)c.b;
        j.kv("hex", hex.str());
        j.end_dict();
    }
    j.end_list();
    j.begin_list_with_key("gradients");
    for (auto i = 0; i < COLOUR_END; i++) {
        if (i != 0) j.f << ",";
        j.f << std::endl << "[";
        for (auto k = 0; k < 8; k++) {
            if (k != 0) j.f << ", ";
            j.f << (int)_colour_gradient[i][k] << " ";
        }
        j.f << "]";
    }
    j.end_list();
    const byte *remap = GetNonSprite(GB(PALETTE_TO_RED, 0, PALETTE_WIDTH), SpriteType::Recolour) + 1;
}

void WriteEngineInfo(JsonWriter &j) {
    j.begin_list_with_key("engines");
    const Engine *e;
    for (const Engine *e : Engine::Iterate()) {
        if (e->type != VEH_TRAIN) continue;
        j.begin_dict();
        JKV(j, e->index);
        j.kv("name", e->name);
        j.kv("cost", e->GetCost());
        j.kv("running_cost", e->GetRunningCost());
        {
            j.begin_dict_with_key("info");
            JKV(j, e->info.cargo_type);
            JKV(j, e->info.cargo_age_period);
            JKV(j, e->info.climates);
            JKV(j, e->info.base_intro.base());
            JKV(j, e->info.lifelength.base());
            JKV(j, e->info.base_life.base());
            JKV(j, e->info.refit_mask);
            JKV(j, e->info.refit_cost);
            JKV(j, e->info.load_amount);
            j.ks("name", e->info.string_id);
            j.end_dict();
        }
        {
            const RailVehicleInfo *rvi = &e->u.rail;
            j.begin_dict_with_key("rail");
            JKV(j, rvi->image_index);
            JKV(j, rvi->railveh_type);
            JKV(j, rvi->railtype);
            JKV(j, rvi->max_speed);
            JKV(j, rvi->power);
            JKV(j, rvi->weight);
            JKV(j, rvi->running_cost);
            JKV(j, rvi->running_cost_class);
            JKV(j, rvi->engclass);
            JKV(j, rvi->tractive_effort);
            JKV(j, rvi->air_drag);
            JKV(j, rvi->capacity);
            SpriteID sprite = GetDefaultTrainSprite(e->original_image_index, DIR_W);
            JKV(j, sprite);
            if (rvi->railveh_type == RAILVEH_MULTIHEAD) {
                j.kv("rev_sprite",
                     GetDefaultTrainSprite(e->original_image_index + 1, DIR_W));
            } else if (rvi->railveh_type == RAILVEH_WAGON) {
                j.kv("full_sprite",
                     sprite + _wagon_full_adder[e->original_image_index]);
            }
            j.end_dict();
        }
        j.end_dict();
    }
    j.end_list();
}

} // namespace export

void ExportOpenttdData(const std::string &filename) {
    data_export::JsonWriter j(filename);
    data_export::WriteHouseSpecInfo(j);
    data_export::WriteCargoSpecInfo(j);
    data_export::WritePaletteInfo(j);
    data_export::WriteEngineInfo(j);
}

} // namespace citymania
