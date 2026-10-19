using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

namespace GrannyLegacyMod;

/// <summary>
/// Carousel: every enemy is frozen and circles the player, following them around.
/// AI scripts and NavMeshAgents are switched off while orbiting and restored afterwards.
/// </summary>
internal static class Orbit
{
    public static bool Enabled;
    public static float Radius = 3f;
    public static float Speed = 1f;  // turns per ~6 s; negative spins the other way

    private sealed class Rider
    {
        public Behaviour Ai;
        public NavMeshAgent Agent;
        public bool AiWasEnabled, AgentWasEnabled;
    }

    private static readonly Dictionary<IntPtr, Rider> Riders = new();
    private static float _angle;

    public static void Apply()
    {
        if (!Enabled)
        {
            if (Riders.Count > 0)
                Release();
            return;
        }

        var cc = Cheats.Fps != null ? Cheats.Fps.characterController : null;
        if (cc == null)
            return;

        // Centre on the player's feet so enemies stand on the same floor.
        var centre = cc.transform.position;
        centre.y = cc.bounds.min.y;

        var active = new List<Cheats.Enemy>();
        foreach (var e in Cheats.EnemyList)
            if (e.Ai != null && e.Ai.gameObject.activeInHierarchy)
                active.Add(e);
        if (active.Count == 0)
            return;

        _angle += Speed * Time.deltaTime;
        for (int i = 0; i < active.Count; i++)
        {
            var e = active[i];
            var rider = Board(e);

            float a = _angle + i * (2f * (float)Math.PI / active.Count);
            var pos = centre + new Vector3((float)Math.Cos(a), 0f, (float)Math.Sin(a)) * Radius;
            var t = e.Ai.transform;
            t.position = pos;

            // Face the player.
            var look = centre - pos;
            look.y = 0f;
            if (look.sqrMagnitude > 0.001f)
                t.rotation = Quaternion.LookRotation(look);
        }
    }

    private static Rider Board(Cheats.Enemy e)
    {
        if (Riders.TryGetValue(e.Ai.Pointer, out var r))
            return r;

        r = new Rider
        {
            Ai = e.Ai,
            Agent = e.Agent,
            AiWasEnabled = e.Ai.enabled,
            AgentWasEnabled = e.Agent != null && e.Agent.enabled,
        };
        e.Ai.enabled = false;
        if (e.Agent != null)
            e.Agent.enabled = false;  // otherwise it snaps the enemy back to its path
        Riders[e.Ai.Pointer] = r;
        return r;
    }

    private static void Release()
    {
        foreach (var r in Riders.Values)
        {
            Cheats.Safe(() =>
            {
                if (r.Agent != null && r.AgentWasEnabled)
                {
                    r.Agent.enabled = true;
                    if (r.Agent.isOnNavMesh)
                        r.Agent.Warp(r.Ai.transform.position);
                }
                if (r.Ai != null && r.AiWasEnabled)
                    r.Ai.enabled = true;
            });
        }
        Riders.Clear();
    }
}
