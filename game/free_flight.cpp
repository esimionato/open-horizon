//
// open horizon -- undefined_darkness@outlook.com
//

#include "free_flight.h"
#include "world.h"

namespace game
{
//------------------------------------------------------------

void free_flight::start(const char *plane, int color, const char *location)
{
    set_plane(plane, color);
    set_location(location);
    m_respawn_time = 0;

    m_world.set_ally_handler(std::bind(&free_flight::is_ally, std::placeholders::_1, std::placeholders::_2));
    update_spawn_pos();
}

//------------------------------------------------------------

void free_flight::update(int dt, const plane_controls &player_controls)
{
    if (!m_player)
        return;

    m_player->controls = player_controls;

    m_world.update(dt);

    if (!m_world.is_host())
        return;

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        if (p->hp <= 0)
        {
            auto respawn_time = p->local_game_data.get<int>("respawn_time");
            if (respawn_time > 0)
            {
                respawn_time -= dt;
                if (respawn_time <= 0)
                    m_world.respawn(p, m_spawn_pos, nya_math::quat());
            }
            else
                respawn_time = 2000;

            p->local_game_data.set("respawn_time", respawn_time);
        }
    }
}

//------------------------------------------------------------

void free_flight::end()
{
    m_player.reset();
    m_world.set_ally_handler(0);
}

//------------------------------------------------------------

void free_flight::set_location(const char *location)
{
    if (!location)
        return;

    m_world.set_location(location);
    if (!m_player)
        return;

    update_spawn_pos();

    m_player->set_pos(m_spawn_pos);
    m_player->set_rot(nya_math::quat());
}

//------------------------------------------------------------

void free_flight::set_plane(const char *plane, int color)
{
    if (!plane)
        return;

    nya_math::vec3 p;
    nya_math::quat r;

    if (m_player)
    {
        p = m_player->get_pos();
        r = m_player->get_rot();
    }

    m_player = m_world.add_plane(plane, m_world.get_player_name(), color, true);
    m_player->set_pos(p);
    m_player->set_rot(r);
}

//------------------------------------------------------------

void free_flight::update_spawn_pos()
{
    const float height_off = 100.0f;
    m_spawn_pos.set(-300.0f, height_off, 2000.0f);

    //adjust start height so first 10 seconds of the flight wil be safe
    for (int i = 0; i < 10; ++i)
    {
        const float mps = 800.0f / 3.6f;
        const float h = m_world.get_height(m_spawn_pos.x, m_spawn_pos.z + mps * i) + height_off;
        if (m_spawn_pos.y < h)
            m_spawn_pos.y = h;
    }
}

//------------------------------------------------------------
}

