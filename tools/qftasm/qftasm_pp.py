import sys
import re




filepath = sys.argv[1]

with open(filepath, "rt") as f:
    text = f.read()

pctable = {}
vartable = {}

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
    else:
        break

# print(pctable)
# print(vartable)

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

