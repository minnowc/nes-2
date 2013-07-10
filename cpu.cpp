#include "cpu.h"

using std::cout;
using std::setw;
using std::hex;
using std::vector;
using std::thread;
using std::string;
using std::ifstream;
using std::exception;
using std::runtime_error;


// CPU cycle chart
static const uint8_t cycles[256] {
//////// 0 1 2 3 4 5 6 7 8 9 A B C D E F
/*0x00*/ 7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
/*0x10*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x20*/ 6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
/*0x30*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x40*/ 6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
/*0x50*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x60*/ 6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
/*0x70*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0x90*/ 2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
/*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0xB0*/ 2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
/*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xD0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0xE0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xF0*/ 2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7
};


uint8_t CPU::read(uint16_t addr){
  if(addr < 0x2000) return memory[addr&0x7ff];
  if(addr < 0x4000){
    auto x = bus::ppu().regr[addr&7]();
    #ifdef DEBUG_PPU
    cout << "Read PPU " << (addr&7) << " --> " << (int)x << '\n';
    #endif
    return x;
  }
  if(addr < 0x4020){
    switch(addr&0x1f){
      case 0x15: return bus::apu().read();
      case 0x16: return bus::io().input_state(1);
      case 0x17: return bus::io().input_state(2);
      default: return 0;
    }
  }
  
  return bus::rom()[addr];
  
}

uint8_t CPU::write(uint8_t value, uint16_t addr){
  if(addr < 0x2000) return memory[addr&0x7ff] = value;
  if(addr < 0x4000){
    #ifdef DEBUG_PPU
    cout << "Write PPU " << (addr&7) << " value " << (int)value << '\n';
    #endif
    return bus::ppu().regw[addr&7](value), 0;
  }
  if(addr < 0x4020){
    switch(addr&0x1f){
      case 0x14: {
        // DMA transfer
        PPU& ppu = bus::ppu();
        for(int i=0; i < 256; ++i){
          ppu.regw[4](read((value&7)*0x100 + i));
        }
      } break;
      case 0x16: 
        if(value&1) 
          bus::io().strobe();
        break;
      default:
        bus::apu().write(value, addr&0x1f);
        break;
    }
  }
  //if(addr < 0x8000){
    // The alternate output...
    /*
    if(!value) cout << '\n';
    cout << hex << std::setw(2) << std::setfill(' ') << value << ' ';
    */
  //}
  
  bus::rom().write(value, addr);
  
}

void CPU::push(uint8_t x){
  memory[0x100 + SP--] = x;
}

void CPU::push2(uint16_t x){
  memory[0x100 + SP--] = (uint8_t)(x >> 8);
  memory[0x100 + SP--] = (uint8_t)(x&0xff);
}

uint8_t CPU::pull(){
  return memory[++SP + 0x100];
}

uint16_t CPU::pull2(){
  uint16_t r = memory[++SP + 0x100];
  return r | (memory[++SP + 0x100] << 8);    
}

void CPU::addcyc(){
  ++result_cycle;
}

uint8_t CPU::next(){
  return read(PC++);
}

uint16_t CPU::next2(){
  uint16_t v = (uint16_t)read(PC++);
  return v | ((uint16_t)read(PC++) << 8);
}

CPU::CPU():memory(0x800, 0xff){
  memory[0x008] = 0xf7;
  memory[0x009] = 0xef;
  memory[0x00a] = 0xdf;
  memory[0x00f] = 0xbf;
  memory[0x1fc] = 0x69;
}

void CPU::pull_NMI(){
  push2(PC);
  stack_push<&CPU::ProcStatus>();
  PC = read(0xfffa) | (read(0xfffb) << 8);
}

void CPU::run(){

  PC = read(0xfffc) | (read(0xfffd) << 8);
  
  int SL = 0;
  
  for(;;){

    last_PC = PC;
    last_op = next();
    
#ifdef DEBUG_CPU
    print_status();
#endif
    
    (this->*ops[last_op])();
    
    if(IRQ==0 && P&I_FLAG==0){
      push2(PC);
      stack_push<&CPU::ProcStatus>();
      P |= I_FLAG;
      PC = read(0xfffe) | (read(0xffff) << 8);
    }

    for(int i = 0; i < cycles[last_op] + result_cycle; ++i){
      bus::ppu().tick3();
      bus::apu().tick();
    }
    
    /*
    result_cycle += cycles[last_op];
    if(result_cycle != test_cyc){
      cout << "On 0x" << hex << (int)last_op << " (" << opasm[last_op] << "): "
        << test_cyc << " should be " << result_cycle << "\n";
      //break;
    }
    */
    test_cyc = 0;
    result_cycle = 0;
  }

}

template<> uint8_t& CPU::getref<&CPU::ACC>(){ return A; }
template<> uint8_t& CPU::getref<&CPU::X__>(){ return X; }
template<> uint8_t& CPU::getref<&CPU::Y__>(){ return Y; }
template<> uint8_t CPU::read<&CPU::IMM>(){
  return (uint8_t)IMM();
}

#include "cpu-map.cc"
#include "cpu-asm.cc"

void CPU::load_state(State const& state){
  P = state.P;
  A = state.A;
  X = state.X;
  Y = state.Y;
  SP = state.SP;
  PC = state.PC;
  result_cycle = state.result_cycle;
  memory = state.cpu_memory;
}

void CPU::save_state(State& state) const {
  state.P = P;
  state.A = A;
  state.X = X;
  state.Y = Y;
  state.SP = SP;
  state.PC = PC;
  state.result_cycle = result_cycle;
  state.cpu_memory = memory;
}


void CPU::print_status(){
  cout 
  << hex << std::uppercase << std::setfill('0')
  << setw(4) << last_PC << "  "
  << setw(2) << (int)last_op << "   "
  << std::setfill(' ') << setw(16) 
  << std::left << opasm[last_op]
  << std::setfill('0')
  << " A:" << setw(2) << (int)A
  << " X:" << setw(2) << (int)X
  << " Y:" << setw(2) << (int)Y
  << " P:" << setw(2) << (int)P
  << " SP:" << setw(2) << (int)SP
  << std::setfill(' ')
  << " CYC:" << setw(3) << std::dec << (int)cyc
  << " SL:" << setw(3) << (int)bus::ppu().scanline
  << hex << std::setfill('0')
  << " ST0:" << setw(2) << (int)memory[0x101 + SP]
  << " ST1:" << setw(2) << (int)memory[0x102 + SP]
  << " ST2:" << setw(2) << (int)memory[0x103 + SP]
  << '\n';
  //if(cyc >= 341) cyc -= 341;
}

