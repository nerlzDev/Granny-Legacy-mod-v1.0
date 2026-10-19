using System;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.AI;

namespace GrannyLegacyMod;

/// <summary>Actions from the enemy wheel (hold X on an enemy). Ids match RadialAction in menu.cpp.</summary>
internal static class EnemyWheel
{
    public const int Freeze = 1, Unfreeze = 2, KnockOut = 3, Pepper = 4, Ice = 5,
                     TeleportTo = 6, Bring = 7, Fling = 8, Despawn = 9, RespawnAll = 10;

    private static readonly List<(Behaviour ai, NavMeshAgent agent)> FrozenOne = new();
    private static readonly List<GameObject> Despawned = new();

    public static int IdOf(Behaviour ai)
    {
        long p = ai.Pointer.ToInt64();
        return (int)(p ^ (p >> 32));
    }

    public static void Run(int action, int targetId) => Cheats.Safe(() =>
    {
        if (action == RespawnAll)
        {
            foreach (var go in Despawned)
                if (go != null)
                    go.SetActive(true);
            Despawned.Clear();
            return;
        }

        Cheats.Enemy target = null;
        foreach (var e in Cheats.EnemyList)
            if (e.Ai != null && IdOf(e.Ai) == targetId)
                target = e;
        if (target == null)
            return;

        var ai = target.Ai;
        var agent = target.Agent;
        var granny = ai.TryCast<AI_Granny>();
        var grandpa = ai.TryCast<AI_Grandpa>();

        switch (action)
        {
            case Freeze:
                if (ai.enabled)
                {
                    ai.enabled = false;
                    if (agent != null && agent.enabled && agent.isOnNavMesh) agent.isStopped = true;
                    FrozenOne.Add((ai, agent));
                }
                break;

            case Unfreeze:
                ai.enabled = true;
                if (agent != null && agent.enabled && agent.isOnNavMesh) agent.isStopped = false;
                FrozenOne.RemoveAll(f => f.ai == null || f.ai.Pointer == ai.Pointer);
                break;

            case KnockOut:
                if (granny != null) Cheats.KnockOutGranny();
                else if (grandpa != null) Cheats.KnockOutGrandpa();
                else DespawnOne(ai);
                break;

            case Pepper:
                granny?.PepperHit();
                grandpa?.PepperHit();
                break;

            case Ice:
                granny?.FrozenEnemy();
                grandpa?.FrozenEnemy();
                break;

            case TeleportTo:
            {
                var player = Cheats.PlayerPosition();
                var cc = Cheats.Fps != null ? Cheats.Fps.characterController : null;
                if (player == null || cc == null) break;
                var to = ai.transform.position;
                var away = player.Value - to;
                away.y = 0;
                var dest = to + (away.sqrMagnitude > 0.01f ? away.normalized : Vector3.forward) * 1.5f;
                cc.enabled = false;
                cc.transform.position = dest + Vector3.up * 0.1f;
                cc.enabled = !Cheats.Noclip;
                break;
            }

            case Bring:
            {
                var cam = Cheats.PlayerCamera;
                var player = Cheats.PlayerPosition();
                if (player == null) break;
                var fwd = cam != null ? cam.transform.forward : Vector3.forward;
                fwd.y = 0;
                MoveEnemy(ai, agent, player.Value + fwd.normalized * 2.5f);
                break;
            }

            case Fling:
            {
                var from = ai.transform.position;
                var player = Cheats.PlayerPosition() ?? from;
                var dir = from - player;
                dir.y = 0;
                if (dir.sqrMagnitude < 0.01f) dir = Vector3.forward;
                MoveEnemy(ai, agent, from + dir.normalized * 20f);
                break;
            }

            case Despawn:
                DespawnOne(ai);
                break;
        }
    });

    // Put an enemy on the nearest walkable spot to `where` (works with or without a NavMeshAgent).
    private static void MoveEnemy(Behaviour ai, NavMeshAgent agent, Vector3 where)
    {
        if (NavMesh.SamplePosition(where, out var hit, 25f, -1))
            where = hit.position;
        if (agent != null && agent.enabled && agent.isOnNavMesh)
            agent.Warp(where);
        else
            ai.transform.position = where;
    }

    private static void DespawnOne(Behaviour ai)
    {
        var go = ai.gameObject;
        go.SetActive(false);
        Despawned.Add(go);
    }
}
