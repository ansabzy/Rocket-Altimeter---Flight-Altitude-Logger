"""
plot_comparison.py

Compares my OpenRocket sim (theoretical altitude) to what the BMP280
actually logged during the flight. Basically: how close was reality to
the simulation?

Inputs:
  1. sim CSV    -> exported straight out of OpenRocket (File > Export flight
                   data, check Time + Altitude). I didn't bother renaming the
                   columns, so this script just looks for whatever column has
                   "time" or "altitude" in the name instead of hardcoding
                   OpenRocket's exact header text (it's a little different
                   depending on version/units anyway).
  2. flight CSV -> flight.csv off the SD card, this is exactly what
                   rocket_altimeter.ino writes (time_ms, pressure_hPa,
                   altitude_m).

Run it like:
    python plot_comparison.py openrocket_sim.csv flight.csv
"""

import sys
import pandas as pd
import matplotlib.pyplot as plt


def find_column(df, keyword):
    # doing a "contains" match instead of an exact column name because
    # OpenRocket's header text isn't consistent (e.g. "Time (s)" vs just
    # "Time"), so matching exact strings kept breaking on me
    matches = [c for c in df.columns if keyword.lower() in c.lower()]
    if not matches:
        raise ValueError(
            f"Couldn't find a column with '{keyword}' in it. "
            f"Columns in the file were: {list(df.columns)}"
        )
    return matches[0]


def load_sim(path):
    # OpenRocket sticks a bunch of comment lines in the export (metadata at
    # the top, plus stuff like "# Event APOGEE occurred at t=..." scattered
    # through the actual data rows). comment="#" makes pandas skip all of
    # those wherever they show up, not just at the top of the file.
    df = pd.read_csv(path, comment="#")
    t_col = find_column(df, "time")
    h_col = find_column(df, "altitude")
    df = df[[t_col, h_col]].apply(pd.to_numeric, errors="coerce").dropna()
    # assuming OpenRocket was set to export in seconds/meters here. if you
    # exported in imperial by accident this will be feet and the comparison
    # will look way off
    return df[t_col].to_numpy(), df[h_col].to_numpy()


def load_flight(path):
    df = pd.read_csv(path, comment="#")
    t_col = find_column(df, "time")
    h_col = find_column(df, "altitude")
    # the sketch appends an "APOGEE_M,xxx" line at the very end of the file
    # as a summary. that row has text in the time column so it just fails
    # to convert to a number and gets dropped here, which is convenient
    df = df[[t_col, h_col]].apply(pd.to_numeric, errors="coerce").dropna()
    t = df[t_col].to_numpy() / 1000.0  # sketch logs time in ms, not s
    alt = df[h_col].to_numpy()

    # the sketch starts logging as soon as it powers on, not at actual
    # liftoff, so there's a bunch of "sitting on the pad" data at the start.
    # find the first big jump in altitude and call that t=0 instead
    launch_idx = 0
    for i in range(1, len(alt)):
        if alt[i] > 5.0:
            launch_idx = i
            break
    t = t - t[launch_idx]
    return t, alt


def main():
    if len(sys.argv) != 3:
        print("Usage: python plot_comparison.py <sim_csv> <flight_csv>")
        sys.exit(1)

    sim_path, flight_path = sys.argv[1], sys.argv[2]

    t_sim, h_sim = load_sim(sim_path)
    t_flight, h_flight = load_flight(flight_path)

    apogee_sim = h_sim.max()
    apogee_flight = h_flight.max()
    pct_diff = (apogee_flight - apogee_sim) / apogee_sim * 100

    fig, ax = plt.subplots(figsize=(9, 6))
    ax.plot(t_sim, h_sim, label=f"OpenRocket sim (apogee {apogee_sim:.1f} m)",
            color="#1f77b4", linewidth=2)
    ax.plot(t_flight, h_flight, label=f"Logged flight data (apogee {apogee_flight:.1f} m)",
            color="#d62728", linewidth=1, alpha=0.85)

    # dashed lines just to make it easy to eyeball how far apart the two
    # apogees actually are
    ax.axhline(apogee_sim, color="#1f77b4", linestyle="--", linewidth=0.7, alpha=0.5)
    ax.axhline(apogee_flight, color="#d62728", linestyle="--", linewidth=0.7, alpha=0.5)

    ax.set_xlabel("Time since liftoff (s)")
    ax.set_ylabel("Altitude (m)")
    ax.set_title("Simulated vs. Logged Altitude")
    ax.legend(loc="upper right")
    ax.grid(alpha=0.3)

    textstr = f"Apogee difference: {pct_diff:+.1f}%"
    ax.text(0.02, 0.95, textstr, transform=ax.transAxes, fontsize=10,
            verticalalignment="top", bbox=dict(boxstyle="round", facecolor="white", alpha=0.8))

    plt.tight_layout()
    out_file = "altitude_comparison.png"
    plt.savefig(out_file, dpi=150)
    print(f"Saved plot to {out_file}")
    print(f"Sim apogee: {apogee_sim:.1f} m | Flight apogee: {apogee_flight:.1f} m | Diff: {pct_diff:+.1f}%")


if __name__ == "__main__":
    main()
