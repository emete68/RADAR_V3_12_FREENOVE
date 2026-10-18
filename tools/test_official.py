import unittest, sys, csv, struct, zlib, hashlib, tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
import import_official as official
import build_radar_db as binary
ROOT=Path(__file__).resolve().parents[1]
class OfficialTests(unittest.TestCase):
 def test_sct_first_location(self):
  rows,bad=official.parse_sct((ROOT/'validation/official/sct-radars.txt').read_bytes())
  self.assertEqual(len(bad),17)
  first=rows[0];self.assertEqual(first['road'],'A-2');self.assertEqual(first['speed'],120)
  self.assertTrue(41.5<first['latitude']<41.6);self.assertTrue(.45<first['longitude']<.5)
  self.assertTrue(all(.0<=r['longitude']<=3.5 for r in rows))
 def test_sct_rejects_broken_coordinate(self):
  raw=b'(ETRS89)\nA-2         501,5-506,5 120         3401992,722 4612186,153 \n'
  a,b=official.parse_sct(raw);self.assertFalse(a);self.assertEqual(len(b),1)
 def test_euskadi_reference(self):
  rows,bad=official.parse_euskadi((ROOT/'validation/official/euskadi-page.html').read_bytes())
  r=next(r for r in rows if r['original_id']=='MAX CENTER')
  self.assertTrue(43.27<r['latitude']<43.29);self.assertTrue(-3.001<r['longitude']<-2.999)
  self.assertEqual(r['speed'],80);self.assertFalse(bad)
 def test_dgt_no_invented_limits(self):
  rows,bad,date=official.parse_dgt((ROOT/'validation/official/dgt-radars.xml').read_bytes())
  self.assertGreater(len(rows),700);self.assertFalse(bad);self.assertTrue(date.startswith('2025-12-18'))
  self.assertTrue(all(r['speed']==0 and r['direction']=='' for r in rows))
  r=next(r for r in rows if r['name']=='GUID_CVM_161274 fin')
  self.assertAlmostEqual(r['latitude'],41.6192)
 def test_binary_matches_every_normalized_record(self):
  blob=(ROOT/'data/radars.bin').read_bytes();magic,version,count,crc=binary.HEADER.unpack(blob[:20])
  self.assertEqual((magic,version,count),(b'RDR312\0\0',1,1075));self.assertEqual(len(blob),20+count*28)
  self.assertEqual(zlib.crc32(blob[20:]),crc)
  expected=sorted(set(binary.parse_file(ROOT/'database/official.csv')))
  self.assertEqual(blob[20:],b''.join(expected))
  ids=[binary.RECORD.unpack(r)[-1] for r in expected];self.assertEqual(len(ids),len(set(ids)))
 def test_invalid_csv_coordinates(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td)/'bad.csv';p.write_text('id,lat,lon\nx,nan,2\n')
   with self.assertRaises(ValueError):list(binary.parse_file(p))
 def test_preserved_original_firmware(self):
  import json
  hashes=json.loads((ROOT/'validation/source-provenance.json').read_text())['original_firmware_hashes']
  if not (ROOT.parent/'firmware').exists():self.skipTest('Original conservado en el workspace, fuera del ZIP')
  for name,digest in hashes.items():self.assertEqual(hashlib.sha256((ROOT.parent/name).read_bytes()).hexdigest(),digest,name)
if __name__=='__main__':unittest.main(verbosity=2)
