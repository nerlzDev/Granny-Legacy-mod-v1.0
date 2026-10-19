using System;
using System.Collections.Generic;
using BepInEx.Logging;

namespace GrannyLegacyMod;

/// <summary>Keeps the last log lines (from every BepInEx source, Unity included) for the overlay Console tab.</summary>
internal sealed class ConsoleLog : ILogListener
{
    private const int MaxLines = 32;

    private static readonly object Lock = new();
    private static readonly List<string> Lines = new();
    private static bool _changed;

    public LogLevel LogLevelFilter => LogLevel.All;

    public static void Install() => Logger.Listeners.Add(new ConsoleLog());

    public void LogEvent(object sender, LogEventArgs e)
    {
        if ((e.Level & (LogLevel.Debug)) != 0)
            return;
        var text = $"[{e.Level,-7}:{e.Source.SourceName}] {e.Data}".Replace('\n', ' ').Replace('\r', ' ');
        lock (Lock)
        {
            Lines.Add(text);
            if (Lines.Count > MaxLines)
                Lines.RemoveAt(0);
            _changed = true;
        }
    }

    /// <summary>Returns a copy of the lines if anything was logged since the last call.</summary>
    public static bool TakeChanged(out List<string> lines)
    {
        lock (Lock)
        {
            lines = _changed ? new List<string>(Lines) : null;
            bool changed = _changed;
            _changed = false;
            return changed;
        }
    }

    public void Dispose() { }
}
