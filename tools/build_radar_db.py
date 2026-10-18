#!/usr/bin/env python3
"""Convierte uno o varios CSV normalizados en data/radars.bin.

Columnas admitidas (los alias se detectan automáticamente):
  id, latitude/lat/latitud, longitude/lon/longitud,
  speed/velocidad/limite, type/tipo, direction/direccion/rumbo

No descarga fuentes de terceros: conserva trazabilidad y permite revisar cada
CSV oficial antes de incorporarlo al dispositivo.

El formato debe coincidir exactamente con RadarRecord de include/RadarTypes.h:
cabecera de 20 bytes seguida de registros compactos de 28 bytes, CRC32 incluido.
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import struct
import unicodedata
import zlib

# `<` fija little-endian; nunca depende de la arquitectura del PC que genere
# la base. Las constantes son también la especificación del archivo binario.
HEADER = struct.Struct("<8sIII")
RECORD = struct.Struct("<iiHBB16s")
ALIASES = {
    "id": ("id", "codigo", "identificador"),
    "lat": ("latitude", "lat", "latitud"),
    "lon": ("longitude", "lon", "lng", "longitud"),
    "speed": ("speed", "velocidad", "limite", "speed_kph"),
    "type": ("type", "tipo"),
    "direction": ("direction", "direccion", "rumbo"),
}
TYPE_CODES = {"fijo": 1, "tramo": 2, "semaforo": 3, "movil": 4, "otro": 0}


def normalized(value: str) -> str:
    """Normaliza encabezados: admite `Velocidad`, `velocidad` y `velocídad`."""
    value = unicodedata.normalize("NFKD", value.strip().lower())
    return "".join(char for char in value if not unicodedata.combining(char))


def find_column(fieldnames: list[str], logical_name: str, required: bool) -> str | None:
    """Devuelve el encabezado físico asociado a un nombre lógico o falla claro."""
    mapping = {normalized(name): name for name in fieldnames}
    for alias in ALIASES[logical_name]:
        if alias in mapping:
            return mapping[alias]
    if required:
        raise ValueError(f"Falta columna {logical_name}; disponibles: {fieldnames}")
    return None


def parse_file(path: pathlib.Path):
    """Lee CSV con coma, punto y coma o tabulador y produce registros binarios."""
    with path.open("r", encoding="utf-8-sig", newline="") as source:
        sample = source.read(4096)
        source.seek(0)
        dialect = csv.Sniffer().sniff(sample, delimiters=",;\t")
        reader = csv.DictReader(source, dialect=dialect)
        if not reader.fieldnames:
            raise ValueError(f"{path}: CSV sin cabecera")
        columns = {
            key: find_column(reader.fieldnames, key, key in {"lat", "lon"})
            for key in ALIASES
        }
        for index, row in enumerate(reader, start=2):
            try:
                lat = float(row[columns["lat"]].replace(",", "."))
                lon = float(row[columns["lon"]].replace(",", "."))
                if not (-90 <= lat <= 90 and -180 <= lon <= 180):
                    raise ValueError("coordenadas fuera de rango")
                speed = int(float(row.get(columns["speed"], "0") or 0)) if columns["speed"] else 0
                direction_text = row.get(columns["direction"], "") if columns["direction"] else ""
                # 255 significa «rumbo no publicado». Los demás rumbos se
                # almacenan divididos por dos para caber en un solo byte.
                direction = 255 if not direction_text.strip() else round(float(direction_text) / 2) % 180
                type_text = normalized(row.get(columns["type"], "otro")) if columns["type"] else "otro"
                radar_id = row.get(columns["id"], f"{path.stem}-{index}") if columns["id"] else f"{path.stem}-{index}"
                yield RECORD.pack(round(lat * 1e7), round(lon * 1e7),
                                  max(0, min(speed, 65535)), TYPE_CODES.get(type_text, 0),
                                  direction, radar_id.encode("ascii", "replace")[:15].ljust(16, b"\0"))
            except (TypeError, ValueError) as error:
                raise ValueError(f"{path}:{index}: {error}") from error


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inputs", nargs="+", type=pathlib.Path)
    parser.add_argument("-o", "--output", type=pathlib.Path,
                        default=pathlib.Path(__file__).parents[1] / "data" / "radars.bin")
    args = parser.parse_args()
    # Orden fijo y eliminación de duplicados hacen que la misma entrada genere
    # exactamente el mismo binario, lo que facilita auditorías de hashes.
    records = sorted(set(record for path in args.inputs for record in parse_file(path)))
    payload = b"".join(records)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(HEADER.pack(b"RDR312\0\0", 1, len(records), zlib.crc32(payload)) + payload)
    print(f"{args.output}: {len(records)} radares, {args.output.stat().st_size} bytes")


if __name__ == "__main__":
    main()

