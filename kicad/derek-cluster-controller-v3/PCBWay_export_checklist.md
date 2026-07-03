# PCBWay Export Checklist

After final KiCad routing:

1. File > Fabrication Outputs > Gerbers
2. File > Fabrication Outputs > Drill Files
3. File > Fabrication Outputs > Component Placement (.pos)
4. Export BOM from KiCad or plugin and split into:
   - PCBWay SMT BOM
   - Hand Assembly BOM
5. In PCBWay quote, select SMT assembly only for the SMT side/components you want installed.
6. Do not include plug-in modules in the PCBWay SMT BOM unless you have confirmed PCBWay can source/install them.
7. Upload Gerbers, drill files, BOM, and CPL/position file.
8. Review PCBWay's online render and assembly preview carefully.
