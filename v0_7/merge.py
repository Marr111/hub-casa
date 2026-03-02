import os

gui_files = ['gui_icons.h', 'gui_widgets.h', 'gui_home.h']
out_lines = ['#ifndef GUI_FUNCTIONS_H\n', '#define GUI_FUNCTIONS_H\n\n', '#include "config.h"\n\n']

for f in gui_files:
    if os.path.exists(f):
        with open(f, 'r', encoding='utf-8') as fin:
            lines = fin.readlines()
            for line in lines:
                if line.startswith('#ifndef') or line.startswith('#define') or line.startswith('#include') or line.startswith('#endif'):
                    continue
                out_lines.append(line)
        os.remove(f)

with open('gui_functions.h', 'w', encoding='utf-8') as fout:
    fout.writelines(out_lines)
    fout.write('\n#endif // GUI_FUNCTIONS_H\n')
print('gui_functions.h merged and split files deleted')
