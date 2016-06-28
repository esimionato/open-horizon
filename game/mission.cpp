//
// open horizon -- undefined_darkness@outlook.com
//

#include "mission.h"
#include "extensions/zip_resources_provider.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

namespace { mission *current_mission = 0; }

void mission::start(const char *plane, int color, const char *mission)
{
    nya_resources::zip_resources_provider zprov;
    if (!zprov.open_archive(mission))
        return;

    pugi::xml_document doc;
    if (!load_xml(zprov.access("objects.xml"), doc))
        return;

    auto root = doc.first_child();

    m_world.set_location(root.attribute("location").as_string());

    auto p = root.child("player");
    if (p)
    {
        m_player = m_world.add_plane(plane, m_world.get_player_name(), color, true);
        m_player->set_pos(read_vec3(p));
        m_player->set_rot(quat(0.0, angle_deg(p.attribute("yaw").as_float()), 0.0));
    }

    world::is_ally_handler fia = std::bind(&mission::is_ally, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);

    auto script_res = zprov.access("script.lua");
    if (script_res)
    {
        std::string script;
        script.resize(script_res->get_size());
        if (!script.empty())
            script_res->read_all(&script[0]);
        m_script.load(script);
        script_res->release();
    }

    m_script.add_callback("start_timer", start_timer);
    m_script.add_callback("setup_timer", setup_timer);
    m_script.add_callback("stop_timer", stop_timer);

    m_script.add_callback("set_hud_visible", set_hud_visible);

    m_script.add_callback("mission_clear", mission_clear);
    m_script.add_callback("mission_fail", mission_fail);

    m_finished = false;
    current_mission = this;
    m_script.call("init");
}

//------------------------------------------------------------

void mission::update(int dt, const plane_controls &player_controls)
{
    if (!m_player)
        return;

    m_player->controls = player_controls;
    current_mission = this;

    static std::vector<std::pair<std::string, std::string> > to_call;
    to_call.clear();
    for (auto &t: m_timers)
    {
        if (!t.active)
            continue;

        if ((t.time -= dt) > 0)
            continue;

        t.time = 0;
        t.active = false;
        to_call.push_back({t.func, t.id});
    }

    for (auto &t: to_call)
        m_script.call(t.first, {t.second});

    m_world.update(dt);

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        if (p->hp <= 0)
            mission_fail(0);
    }

    m_world.get_hud().clear_texts();
    int last_text_idx = 0;
    for (auto &t: m_timers)
    {
        if (!t.active || t.name.empty())
            continue;

        int s = t.time / 1000;
        wchar_t buf[255];
        swprintf(buf, sizeof(buf), L": %d:%02d", s/60, s % 60);

        m_world.get_hud().add_text(last_text_idx, t.name + buf, "Zurich20", 1000, 150 + last_text_idx * 30, gui::hud::green);
        ++last_text_idx;
    }

    if (m_hide_hud)
        m_world.get_hud().set_hide(true);
}

//------------------------------------------------------------

void mission::end()
{
    m_player.reset();
}

//------------------------------------------------------------

int mission::start_timer(lua_State *state)
{
    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t == current_mission->m_timers.end())
        t = current_mission->m_timers.insert(t, timer()), t->id = id;

    t->time = script::get_int(state, 1) * 1000;
    t->func = script::get_string(state, 2);
    t->active = true;
    return 0;
}

//------------------------------------------------------------

int mission::setup_timer(lua_State *state)
{
    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t == current_mission->m_timers.end())
        t = current_mission->m_timers.insert(t, timer()), t->id = id;

    t->name = to_wstring(script::get_string(state, 1));
    return 0;
}

//------------------------------------------------------------

int mission::stop_timer(lua_State *state)
{
    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t != current_mission->m_timers.end())
        t->active = false;

    return 0;
}

//------------------------------------------------------------

int mission::set_hud_visible(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_hide_hud = script::get_int(state, 0) == 0;
    return 0;
}

//------------------------------------------------------------

int mission::mission_clear(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_world.popup_mission_clear();
    current_mission->m_finished = true;
    return 0;
}

//------------------------------------------------------------

int mission::mission_fail(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_world.popup_mission_fail();
    current_mission->m_finished = true;
    return 0;
}

//------------------------------------------------------------
}