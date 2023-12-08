import struct
from pathlib import Path

from util import options
from segtypes.n64.segment import N64Segment

class N64SegFont(N64Segment):
    def __init__(self, rom_start, rom_end, type, name, vram_start, args, yaml):
        super().__init__(
            rom_start, rom_end, type, name, vram_start, args=args, yaml=yaml
        )
        self.data_only = True

    def out_path(self) -> Path:
        return options.opts.asset_path / self.dir / f"{self.name}.font.inc.c"

    def disassemble_data(self, rom_bytes) -> str:
        assert isinstance(self.rom_start, int)
        assert isinstance(self.rom_end, int)

        font_data = rom_bytes[self.rom_start : self.rom_end]
        segment_length = len(font_data)
        if (segment_length) % 8 != 0:
            log.error(
                f"Error: Font segment {self.name} length ({segment_length}) is not a multiple of 8!"
            )

        lines = []
        num = 0
        font_string = ""
        for font in struct.iter_unpack(">I", font_data):
            font_string += f"0x{font[0]:08X}, "
            num += 1
            if num == 10:
                lines.append(font_string)
                font_string = ""
                num = 0

        lines.append(font_string)
        # enforce newline at end of file
        lines.append("")
        return "\n".join(lines)

    def scan(self, rom_bytes: bytes):        
        self.file_text = self.disassemble_data(rom_bytes)
    
    def split(self, rom_bytes: bytes):
        if self.file_text and self.out_path():
            self.out_path().parent.mkdir(parents=True, exist_ok=True)

            with open(self.out_path(), "w", newline="\n") as f:
                f.write(self.file_text)