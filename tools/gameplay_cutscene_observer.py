"""Compose the existing route observer with the shared cutscene capture commands."""
import argparse
from pathlib import Path
from extract_original_data import ROOT


def generate():
    route = (ROOT / 'amiga/gameplay_route.gdb').read_text()
    captures = (ROOT / 'amiga/natural_opening.gdb').read_text()
    marker = 'tbreak pop2CoreSegmentLoaded'
    assert route.endswith('continue\n') and captures.count(marker) == 1
    return route[:-len('continue\n')] + marker + captures.split(marker, 1)[1]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(generate())
