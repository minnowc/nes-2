#include "bus.h"
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"

namespace bus {
  
  IO _io;
  PPU _ppu;
  CPU _cpu;
  APU _apu;
  ROM _rom;

  
  PPU& ppu(){    
    return _ppu;
  }
  
  CPU& cpu(){
    return _cpu;
  }
  
  APU& apu(){
    return _apu;
  }
  
  ROM& rom(){
    return _rom;
  }
  
  IO& io(){
    return _io;
  }
  
  void pull_NMI(){
    _cpu.pull_NMI();
  }
  
  void play(std::string const& path){
    _rom = ROM(path);
  }
  
  State state1;
  
  void save_state(){
    _ppu.save_state(state1);
    _cpu.save_state(state1);
  }
  
  void restore_state(){
    _ppu.load_state(state1);
    _cpu.load_state(state1);
  }
  
}
