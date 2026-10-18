"""Descarga/normaliza exclusivamente los tres recursos oficiales, con auditoría.
Uso: python tools/import_official.py [--download]; requiere pyproj==3.7.2.
No corrige números mal formados ni deduce límites o sentidos no publicados.

El argumento --download cambia los ficheros de evidencia de validation/official.
Sin él, permite repetir la conversión exactamente con la descarga ya archivada.
"""
from pathlib import Path
import argparse, csv, hashlib, json, re, sys, urllib.request
import xml.etree.ElementTree as ET
from datetime import datetime, timezone
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'validation/python_deps'))
from pyproj import Transformer
# Fuentes primarias explícitas. No se consulta una base de radares de terceros.
URLS={
 'dgt-radars.xml':'https://infocar.dgt.es/datex2/dgt/PredefinedLocationsPublication/radares/content.xml',
 'sct-radars.txt':'https://transit.gencat.cat/web/.content/documents/seguretat_viaria/radars.txt',
 'euskadi-page.html':'https://apps.trafikoa.euskadi.eus/lfr/es/web/trafikoa/cabinas-de-radar-fijo',
 'euskadi-crs.js':'https://apps.trafikoa.euskadi.eus/gis/bisorearina/js/constantes.js'}
NS={'d':'http://datex2.eu/schema/1_0/1_0'}
def text(e,path):return e.findtext(path,default='',namespaces=NS)
def valid(lat,lon):return 27<=lat<=44.5 and -19<=lon<=5
def parse_dgt(raw):
 # DATEX II ya publica latitud/longitud. DGT no aporta aquí límites ni rumbo:
 # se guardan como 0/vacío y la interfaz muestra «--» en vez de inventarlos.
 root=ET.fromstring(raw); rows=[]; rejected=[]
 for loc in root.findall('.//d:predefinedLocation[@id]',NS):
  ident=loc.attrib['id']; typ='tramo' if 'CVM_' in ident else 'fijo'
  points=loc.findall('.//d:pointCoordinates',NS)
  for i,pt in enumerate(points):
   lat=float(text(pt,'d:latitude'));lon=float(text(pt,'d:longitude'))
   if not valid(lat,lon):rejected.append({'source':'DGT','id':ident,'reason':'Fuera de España','lat':lat,'lon':lon});continue
   suffix=('fin' if i==0 else 'ini') if typ=='tramo' else ''
   key='DGT'+hashlib.sha256((ident+suffix).encode()).hexdigest()[:11]
   rows.append(dict(id=key,latitude=lat,longitude=lon,speed=0,type=typ,direction='',source='DGT',road=text(loc,'.//d:roadNumber'),pk=text(loc,'.//d:referencePointDistance'),name=ident+' '+suffix,original_id=ident,crs='DATEX2 latitude/longitude'))
 return rows,rejected,text(root,'.//d:publicationTime')
def parse_sct(raw):
 # SCT publica ETRS89 / UTM 31N. pyproj hace la transformación documentada.
 trans=Transformer.from_crs(25831,4326,always_xy=True);rows=[];rejected=[]
 for n,line in enumerate(raw.decode('utf-8-sig').splitlines(),1):
  if not line.strip() or line.startswith('(ETRS') or line.strip().startswith('Via'):continue
  # Fields are fixed-width (12,12,12,12,...); retain spaces in road and PK.
  fields=[line[0:12].strip(),line[12:24].strip(),line[24:36].strip(),line[36:48].strip(),line[48:].strip()]
  try:
   road,pk,speed,x,y=fields;x=float(x.replace(',','.'));y=float(y.replace(',','.'));speed=int(speed)
   if not(250000<=x<=550000 and 4400000<=y<=4800000):raise ValueError('UTM fuera del rango de Catalunya; sin correccion automatica')
   lon,lat=trans.transform(x,y)
   if not(40<=lat<=43.1 and 0<=lon<=3.5):raise ValueError('Posicion transformada fuera de Catalunya')
   if not 20<=speed<=130:raise ValueError('Limite fuera de rango')
   key='SCT'+hashlib.sha256(line.strip().encode()).hexdigest()[:11]
   rows.append(dict(id=key,latitude=lat,longitude=lon,speed=speed,type='tramo' if '-' in pk else 'fijo',direction='',source='SCT',road=road,pk=pk,name=road+' PK '+pk,original_id='linea '+str(n),crs='EPSG:25831 ETRS89 UTM31N'))
  except ValueError as e:rejected.append(dict(source='SCT',line=n,raw=line,reason=str(e)))
 return rows,rejected
def parse_euskadi(raw):
 # Euskadi publica coordenadas ETRS89 / UTM 30N dentro de un JavaScript web.
 trans=Transformer.from_crs(25830,4326,always_xy=True);rows=[];rejected=[]
 pattern=r'var x = ([\d.\-]+);\s*var y = ([\d.\-]+);(.*?)(?=var x =|function loadMapa|$)'
 for x,y,block in re.findall(pattern,raw.decode('utf-8'),re.S):
  match=re.search(r'var popupValores = (\[[^\n]+?\]);',block)
  if not match:raise ValueError('Euskadi: bloque sin atributos')
  a=json.loads(match.group(1));lon,lat=trans.transform(float(x),float(y))
  if not(42.3<=lat<=43.6 and -3.6<=lon<=-1.6):rejected.append(dict(source='EUS',name=a[0],reason='Fuera de Euskadi',x=x,y=y));continue
  limit=re.search(r'\d+',a[6]);speed=int(limit.group()) if limit else 0
  key='EUS'+hashlib.sha256(('|'.join(a[:7])+x+y).encode()).hexdigest()[:11]
  rows.append(dict(id=key,latitude=lat,longitude=lon,speed=speed,type='tramo' if 'TRAMO' in a[0].upper() else 'fijo',direction='',source='EUS',road=a[4],pk=a[5],name=a[0]+' / '+a[1],original_id=a[0],crs='EPSG:25830 ETRS89 UTM30N'))
 if not rows:raise ValueError('Euskadi: no se encontraron radares')
 return rows,rejected
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--download',action='store_true');args=ap.parse_args()
 rawdir=ROOT/'validation/official';rawdir.mkdir(parents=True,exist_ok=True)
 if args.download:
  for name,url in URLS.items():
   with urllib.request.urlopen(url,timeout=45) as r: payload=r.read()
   if not payload:raise ValueError('Descarga vacia: '+url)
   (rawdir/name).write_bytes(payload)
 rows,rejected,published=parse_dgt((rawdir/'dgt-radars.xml').read_bytes())
 for parser,name in [(parse_sct,'sct-radars.txt'),(parse_euskadi,'euskadi-page.html')]:
  a,b=parser((rawdir/name).read_bytes());rows+=a;rejected+=b
 # Solo se deduplican puntos idénticos de la misma fuente; no se fusionan
 # registros parecidos de administraciones diferentes porque perderían origen.
 unique={};duplicates=[]
 for row in rows:
  key=tuple(row[k] for k in ('source','latitude','longitude','speed','type','direction'))
  if key in unique:duplicates.append(row)
  else:unique[key]=row
 rows=list(unique.values());out=ROOT/'database';out.mkdir(exist_ok=True)
 with (out/'official.csv').open('w',encoding='utf-8',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
 report={'processed_at':datetime.now(timezone.utc).isoformat(),'dgt_publication':published,'counts':{s:sum(r['source']==s for r in rows) for s in ('DGT','SCT','EUS')},'total':len(rows),'rejected':rejected,'duplicates':duplicates,'sources':[{ 'url':url,'file':name,'sha256':hashlib.sha256((rawdir/name).read_bytes()).hexdigest()} for name,url in URLS.items()], 'limitations':['DGT no publica limites de velocidad en este XML: speed=0, interfaz --.','Rumbos desconocidos: direction vacio; solo filtro de aproximacion geometrico, sin ajuste a carretera.','Tramos se representan como puntos publicados; no se calcula velocidad media ni recorrido.','La fecha de descarga no implica actualizacion del contenido.','SCT: conversion ETRS89 UTM31N; entradas anómalas excluidas, sin reparar cifras.']}
 (out/'audit.json').write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
 print(json.dumps({k:report[k] for k in ('counts','total','dgt_publication')},ensure_ascii=False));print('Rechazados',len(rejected),'duplicados',len(duplicates))
if __name__=='__main__':main()
