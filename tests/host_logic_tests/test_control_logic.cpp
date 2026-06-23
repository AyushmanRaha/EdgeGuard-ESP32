#include <cassert>
#include "edgeguard_control.h"

static SensorSnapshot sensor(bool dhtOk=true, bool distanceOk=true, uint16_t cm=50, bool dark=true, float temp=25.0f){ SensorSnapshot s; s.dhtOk=dhtOk; s.distanceOk=distanceOk; s.distanceCm=cm; s.lightIsDark=dark; s.temperatureC=temp; return s; }
static ControlResult run(const SensorSnapshot& s, SystemSnapshot& sys, ControlContext& ctx, uint32_t now){ ControlResult r=updateControlLogic(s, sys, ctx, defaultControlConfig(), now); sys=r.system; return r; }

int main(){
  { SystemSnapshot sys; ControlContext ctx; assert(run(sensor(true,true,50,true),sys,ctx,1000).system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; assert(!run(sensor(true,true,50,false),sys,ctx,1000).system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; run(sensor(true,true,50,true),sys,ctx,1000); assert(!run(sensor(true,true,200,true),sys,ctx,17000).system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; auto r=run(sensor(true,true,120,true),sys,ctx,1000); assert(r.instantOccupied); assert(r.system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; auto r=run(sensor(true,true,121,true),sys,ctx,1000); assert(!r.instantOccupied); assert(!r.system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; run(sensor(true,true,50,true),sys,ctx,1000); assert(run(sensor(true,true,200,true),sys,ctx,5000).system.relay1On); assert(!run(sensor(true,true,200,true),sys,ctx,17000).system.relay1On); }
  { SystemSnapshot sys; ControlContext ctx; assert(run(sensor(true,true,50,true,35.0f),sys,ctx,1000).system.temperatureAlert); }
  { SystemSnapshot sys; ControlContext ctx; assert(run(sensor(true,true,50,true,36.0f),sys,ctx,1000).system.state==State::ALERT); assert(run(sensor(true,true,50,true,34.0f),sys,ctx,2000).system.temperatureAlert); assert(!run(sensor(true,true,50,true,33.0f),sys,ctx,3000).system.temperatureAlert); }
  { SystemSnapshot sys; ControlContext ctx; ControlResult r; for(int i=0;i<5;i++) r=run(sensor(false,true,50,true),sys,ctx,1000+i); assert(r.system.faultCode==FaultCode::DHT); assert(!r.system.relay1On && !r.system.relay2On); }
  { SystemSnapshot sys; ControlContext ctx; ControlResult r; for(int i=0;i<5;i++) r=run(sensor(true,false,0,true),sys,ctx,1000+i); assert(r.system.faultCode==FaultCode::ULTRASONIC); }
  { SystemSnapshot sys; ControlContext ctx; ControlResult r; for(int i=0;i<5;i++) r=run(sensor(false,false,0,true),sys,ctx,1000+i); assert(r.system.faultCode==FaultCode::DHT_AND_ULTRASONIC); }
  { SystemSnapshot sys; sys.mode=Mode::MANUAL; sys.relay1On=true; sys.relay2On=false; ControlContext ctx; auto r=run(sensor(true,true,200,false),sys,ctx,1000); assert(r.system.state==State::MANUAL_OVERRIDE && r.system.relay1On && !r.system.relay2On); }
  { SystemSnapshot sys; sys.mode=Mode::MANUAL; sys.relay1On=true; sys.relay2On=true; ControlContext ctx; ControlResult r; for(int i=0;i<5;i++) r=run(sensor(false,false,0,true),sys,ctx,1000+i); assert(r.system.state==State::FAULT && !r.system.relay1On && !r.system.relay2On); }
  { SystemSnapshot sys; sys.mode=Mode::AWAY; ControlContext ctx; assert(run(sensor(true,true,50,false),sys,ctx,1000).system.relay2On); assert(!run(sensor(true,true,200,false),sys,ctx,2000).system.relay2On); }
  { SystemSnapshot sys; ControlContext ctx; ControlResult r; for(int i=0;i<5;i++) r=run(sensor(false,false,0,true),sys,ctx,1000+i); assert(r.system.state==State::FAULT); ctx.dhtFailCount=0; ctx.ultrasonicFailCount=0; sys.faultCode=FaultCode::NONE; r=run(sensor(true,true,50,true),sys,ctx,3000); assert(r.system.state!=State::FAULT); }
  return 0;
}
