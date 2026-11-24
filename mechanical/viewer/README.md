# Seri-tool 3D viewer

Interactive page with the 3D model (enclosure, board copper, all 116 placed parts, DevKitC, modem, modules,
printer, OLED, probe), the block circuit diagram, the KiCad schematic, and a working demo (mains, power cut,
temperature alarm, upload) that mirrors the tested firmware behaviour.

Rebuild the data after any board or enclosure change, then open `site/index.html` through a local web server:

    python3 mechanical/viewer/build_viewer.py      # writes site/scene.json, enclosure.json, board.json, schematic.svg
    cd mechanical/viewer/site && python3 -m http.server 8000

Published copy: https://claude.ai/artifact/GNvabFMbRdYcCAotZbhTrH (private to the owner until shared).
