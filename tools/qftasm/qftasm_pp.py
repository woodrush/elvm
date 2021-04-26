import sys
import re

filepath = sys.argv[1]

with open(filepath, "rt") as f:
    text = f.read()

text = re.sub(r";;; (\.L\d+.*)", r";; \1", text)

pctable = {}
vartable = {}
pcredirecttable = {}

for line in text.split("\n"):
    # pc table
    if line.startswith(";; "):
        line = line[3:]
        k, v = line.split(" : ")
        pctable[k] = v
        continue
    # varname table
    elif line.startswith(";;; "):
        line = line[4:]
        k, v = line.split(" : ")
        vartable[k] = v
        continue
    # pc redirection table
    elif line.startswith(";;;; "):
        line = line[5:]
        k, v = line.split(" : ")
        pcvalue = k[2:]
        label = v
        label = label.replace("{","")
        label = label.replace("}","")
        pcredirecttable[pcvalue] = label
        continue
    else:
        break

# print(pctable)
# print(vartable)

for k_redirect_middle, v_redirect_dest in pcredirecttable.items():
    for k_pctable_src, v_pctable_middle in pctable.items():
        if v_pctable_middle == k_redirect_middle:
            pctable[k_pctable_src] = pctable[v_redirect_dest]
#             print(k_pctable_src, v_pctable_middle, v_redirect_dest, pctable[v_redirect_dest])
    
# print(pctable)
# exit()
for k, v in pctable.items():
    text = text.replace("{" + k + "}", "{____pc" + v + "}")

for k, v in vartable.items():
    text = text.replace("{" + k + "}", str(v))

lines = [line for line in text.split("\n") if not line.startswith(";;")]

d = {}
for i_line, line in enumerate(lines):
    matches = re.findall(r'pc == ([0-9]+):', line)

    for m in matches:
        d["____pc" + m] = i_line

for k, v in d.items():
    text = text.replace("{" + k + "}", str(v))

# text = text.format(**d)
# text = text[:-1] # Remove the newline at the end


lines = text.split("\n")
for i_line, line in enumerate(lines):
    if line.startswith(";;"):
        continue
    print(line, end="" if i_line == len(lines) - 1 else "\n")

# # print(text, end="")

