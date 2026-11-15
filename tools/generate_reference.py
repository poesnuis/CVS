"""Developer-only fixture regeneration:
pip install git+https://github.com/DaltonLens/DaltonLens-Python.git@3cba5e6a7c8f0e8199c8f83f1afb58eb6dab7a3d
No Python dependency is used by the application or normal build/test.
"""
from pathlib import Path
import importlib.metadata
import json
import numpy as np
from daltonlens import simulate, convert
commit='3cba5e6a7c8f0e8199c8f83f1afb58eb6dab7a3d'
installed=json.loads(importlib.metadata.distribution('daltonlens').read_text('direct_url.json'))
assert installed['vcs_info']['commit_id'] == commit
colors = np.array([[1,0,0],[0,1,0],[0,0,1],[0.2,0.5,0.8],[0.9,0.55,0.45],[0.03,0.04,0.05],[0.7,0.3,0.9],[1,1,1]], dtype=np.float32)
linear = convert.linearRGB_from_sRGB(colors.reshape(1, -1, 3))
rows=[]
for model, sim in [(1,simulate.Simulator_Machado2009()),(2,simulate.Simulator_Brettel1997())]:
 for deficiency in simulate.Deficiency:
  # This DaltonLens revision omits a factor of 10 in Machado interpolation alpha.
  # Use tabulated steps for Machado; CVS tests interpolation independently.
  # Brettel's 0.45 blend remains an independent partial-severity reference.
  for severity in ([0.4, 1.0] if model == 1 else [0.45, 1.0]):
   predicted=sim._simulate_cvd_linear_rgb(linear.copy(),deficiency,severity)
   output=convert.sRGB_from_linearRGB(np.clip(predicted,0,1)).reshape(-1,3)
   for inp,expected in zip(colors,output):
    vals=lambda x: ','.join(f'{float(y):.10f}f' for y in x)
    rows.append(' {'+f'{model},{deficiency.value},{severity:.2f}f,'+'{'+vals(inp)+'},{'+vals(expected)+'}},')
root=Path(__file__).resolve().parents[1]
(root/'tests/reference_vectors.h').write_text('''// Generated independently by DaltonLens 3cba5e6a7c8f (MIT); see tools/generate_reference.py.
#pragma once
struct ReferenceVector { int model, deficiency; float severity; cvs::Vec3 input, expected; };
inline const ReferenceVector referenceVectors[] = {
'''+ '\n'.join(rows)+'\n};\n')
print(f'Wrote {len(rows)} independent reference vectors')
