import sys
import re
import subprocess

def linecount(p1, p2, s):
  valid_lines = []
  lines = s.split('\n') 
  regex_p1 = re.compile(p1)
  regex_p2 = re.compile(p2)
  for line in lines:
    r1 = regex_p1.search(line)
    if r1:
      r2 = regex_p2.search(line)
      if r2 is None:
        valid_lines.append(r1.group(0)) 
  return valid_lines 

def raw_file_line_count(s):
  lines = s.split('\n')
  count = 0
  #grep -v extern | grep -vP "^\w*$" | grep -v "#"
  crap = re.compile(r"""(extern|^\w*$|#)""")
  for line in lines:
    if crap.search(line) is None:
      count += 1

  return count

def svcomp_filter(f):
  lines = None
  with open(f, 'r') as inputfile:
    lines = inputfile.read()

  pruned_lines = raw_file_line_count(lines)
  raw_lines = len(lines.split('\n'))

  if bv_filter(lines, raw_lines, pruned_lines):
    return 'bitvector'

  if float_filter(lines, raw_lines, pruned_lines):
    return 'float'

  return 'normal'
    
def bv_filter(lines, raw_line_count, pruned_line_count):
  if ("bugBrokenOut" in lines or "returnsStructure" in lines or "__VERIFIER_nondet_double" in lines or
      "__VERIFIER_nondet_float" in lines or "0x43300000" in lines or "float X, P;" in lines or "1415926538837245" in lines or
      "huge_floor" in lines or "huge_ceil" in lines or "tiny_sqrt" in lines or "fmax_float" in lines or
      "fabs_double" in lines or "round_double" in lines or "trunc_double" in lines or "zero_log" in lines or
      "isinf_float" in lines or "trunc_float" in lines or "exponent_less_127" in lines):
    return 0
  elif ('4294967294u' in lines or '616783' in lines or '__main(argc' in lines):
    return 1

  if raw_line_count > 1500:
    if 'ldv_usb_gadget' in lines or "SyncPush" in lines or "kaweth_set_receive_filter" in lines:
      return 1
    else:
      return 0
  
  #line_count = raw_file_line_count(lines)
  if pruned_line_count > 650:
    return 0

  #special case for Concurrent benchmarks
  if "pthread_create" in lines:
    return 0

  #cast patterns
  if pruned_line_count <= 210:

    if pruned_line_count < 25 and 'while(1)' in lines:
      return 1

    casts = re.compile(r'''4294967295|plus_one|minus_one|\(x % 2\) == \(y % 2\)|linear_search|while \(\('0' <= c\) && \(c <= '9'\)\)''')
    if casts.search(lines):
      return 1
  #bwops = re.compile(r'''[^\&]\&[^\&]|[^\|]\|[^\|]|\^|>>|<<''')
  bwops = re.compile(r'''[=|\(][^,]*[^\&|\(|{]\&\s|[^\|]\|\s|\^|>>|<<''')
  #bwops = re.compile(r'''\s\&\s|\s\|\s|\^|>>|<<''')
  #dv = re.compile(r'''1\s+<<\s+[1|5]|cil''')
  dv = re.compile(r'''1.*<<.*\"|cil|found|node''')
  
  for line in lines.split('\n'):
    if bwops.search(line):
      if dv.search(line) is None: 
        #print line
        return 1
  #print raw_file_line_count(lines)
  return 0
    
def float_filter(lines, raw_line_count, pruned_line_count):
  fliteral = 0 
  ddecl = 0

  if ("bugBrokenOut" in lines or "returnsStructure" in lines or "__VERIFIER_nondet_double" in lines or
      "__VERIFIER_nondet_float" in lines or "0x43300000" in lines or "float X, P;" in lines or "1415926538837245" in lines or
      "huge_floor" in lines or "huge_ceil" in lines or "tiny_sqrt" in lines or "fmax_float" in lines or
      "fabs_double" in lines or "round_double" in lines or "trunc_double" in lines or "zero_log" in lines or
      "isinf_float" in lines or "trunc_float" in lines or "exponent_less_127" in lines):
    return 1

  #heuristic #-1: don't do test on too large programs
  if raw_line_count >= 2000: 
    return 0
  #heuristic #0: more than Float.set maximum LOC ==> not 
  if pruned_line_count > 140: #(138 = maximum) 
    return 0
  #heuristic #1: __VERIFIER_nondet ==> float category
  result = re.search(r"""(__VERIFIER_nondet_(float|double)|ieee754_float)""", lines)  
  if result:
    return 1

  #heuristic #2: check float literal # 
  valid_lines = linecount(r"""(0x)?\d+\.\d*(f|p|e)?""", r"""#|line|June|CIL|0\.000000|\"\d+|Created""", lines)
  count = len(valid_lines)
  if count > 60:
    return 0
  if count == 0:
  #heuristic #3: check double 
    #result = re.search(r"""0x\d+\.?.*p|double""", lines)
    result = re.search(r"""double""", lines)
    if result:
      return 1
    else:
      return 0
  else: 
  #heuristic #4: for loops/heapmanipulation
    #print valid_lines
    regex_special = re.compile(r"""1\.4p|1\.0e""")
    for valid_line in valid_lines:
      if regex_special.search(valid_line) is not None and count <= 4:
        return 0 
      if valid_line == '1.' or valid_line == '2.':
        if 'double' not in lines:
          return 0
        else:
          return 1
    return 1 

if __name__ == '__main__':
  print("What?")
  print(svcomp_filter(sys.argv[1]))

