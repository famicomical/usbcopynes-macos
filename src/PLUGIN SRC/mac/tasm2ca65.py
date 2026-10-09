import re, sys
out = []
for line in open(sys.argv[1], encoding='latin-1').read().splitlines():
    code, _, comment = line.partition(';')
    if re.match(r'\s*\.(title|end)\b', code): continue
    m = re.match(r'^(\w+):\s+\.equ\s+(\S+)\s*$', code)
    if m: code = f'{m.group(1)} = {m.group(2)}'
    # hex literals outside strings
    parts = re.split(r'("[^"]*")', code)
    for i in range(0, len(parts), 2):
        parts[i] = re.sub(r'\b([0-9][0-9a-fA-F]*)h\b', r'$\1', parts[i])
        parts[i] = parts[i].replace('.db', '.byte').replace('.fill', '.res')
    for i in range(1, len(parts), 2):
        parts[i] = parts[i].replace('\\n', '",10,"').replace('\\r', '",13,"').replace('"",', '').replace(',""', '')
    code = ''.join(parts)
    out.append(code + (';' + comment if _ else ''))
open(sys.argv[2], 'w').write('\n'.join(out) + '\n')
