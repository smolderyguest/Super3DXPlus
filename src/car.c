/* The race on the arcade's own code: CSeq::Init and CSeq::Exec, the arcade's race
 * sequence (the player's car, the traffic and what they hit), recompiled to C
 * (src/phys/phys_rc.c). This file sets the race up and reads back what to draw. */
#include "car.h"
#include "netdiag.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "phys/rc_runtime.h"

void f_89878c4(void); /* yt::CSeq::Init(SConfigPlay const&) */
void f_8983f64(void); /* yt::CSeq::Exec(std::auto_ptr<clGameInput> const&) */
void f_89a2584(void); /* yt::CElapse::Init */
void f_89a2548(void); /* yt::CElapse::SetByMul */
void f_89825a8(void); /* yt::CSeq::GetInfoOtherCarDisp */
void f_89b45d0(void); /* yt::CMachine::GetInfoCarDisp */
void f_89b410c(void); /* yt::CMachine::GetInfoCarEffect */
void f_89ce5a4(void); /* yt::CTire::GetForce(yt::STireInput const&) const, for car_phys */
#ifdef WMMT_EGL
#include <stdlib.h>
void f_89bcca8(void); /* yt::CMcPilot::Drive, for the desktop harness's WMMT_PILOT */
#endif
void f_89ba620(void); /* yt::CMachine::GetInfoCarCamera */
void f_8982580(void); /* yt::CSeq::GetInfoMirrorCamera */
void f_8982338(void); /* yt::CSeq::GetInfoCarCamera */
void f_89c4000(void); /* yt::CAreaProcessorBase::GetInfoJctBlockDisp */
void f_89c50c0(void); /* yt::CSlit::GetFirstPoint(float, float) const */
void f_89ba05c(void); /* yt::CMachine::SetFirstPos(SFirstPoint const&, float) */
void f_8983950(void); /* yt::CSeq::Get2DInfoRace */
void f_8983874(void); /* yt::CSeq::Get2DInfoMap */
void f_898380c(void); /* yt::CSeq::Get2DInfoSign */
void f_8982f4c(void); /* yt::CSeq::Get2DInfoJctSelect */
void f_8983cd8(void); /* yt::CSeq::GetInfoJctDecideLine */
void f_898f518(void); /* yt::CSeq::GetInfoGoalLine */
void f_898f6d4(void); /* yt::CSeq::GetInfoSplitLine */
void f_8983e5c(void); /* yt::CSeq::GetInfoTenRivalLine */
void f_8983488(void); /* yt::CSeq::GetInfoRivalCarDisp */
void f_898287c(void); /* yt::CSeq::Get2DInfoStory */
void f_898317c(void); /* yt::CSeq::GetPlayResult */
void f_898332c(void); /* yt::CSeq::GetPlayLog(youTitle::_TitleRunningLog_ *) */
void f_858d8e8(void); /* youTitle::GetTitleList(_TitleData_ &, _TITLEDATALIST_ &) */
void f_899b378(void); /* yt::CConfigLoader::GetStorySeries(file, int, ESeries), returning SStorySeries */
void f_8982be8(void); /* yt::CSeq::Get2DInfoTenFight(S2DInfoTenFight *) */
void f_89923e4(void); /* yt::GetTenFightRaceField(EStage, level), returning SConfigRaceField */
/* YOURTMOVIE_CPP::clMovieSystem, the realtime movies (clRaceThread::initStory, race) */
void f_85b15a8(void); /* Cut::clCutIn::clCutIn() */
void f_8986878(void); /* yt::CSeq::End() */
void f_85b16a4(void); /* Cut::clCutIn::Init(int, int) */
void f_85b3754(void); /* Cut::clCutIn::Update() */
void f_89fe784(void); /* clMovieSystem::clMovieSystem(unsigned int) */
void f_89fd9d0(void); /* clMovieSystem::Init(eMovieType, ECarType x4), returning the movie's length in frames */
void f_89fe890(void); /* clMovieSystem::UpDate(float frame) */
void f_89fde9c(void); /* clMovieSystem::GetRTMovieCamera(SInfoCarCamera *) */
void f_89fdfe8(void); /* clMovieSystem::GetRTMovieCarDisp(int, SInfoCarDisp *) */

/* arcade enums and data */
#define TRANS_AT 0u
#define TRANS_MT 1u
#define MODE_STORY 4u            /* EGameMode (GetEGameModeName) */
#define MODE_TIME_ATTACK 6u
/* SConfigPlay, what clRaceThread hands CSeq::Init */
#define CFG_CAR 0x00u
#define CFG_POWER 0x08u
#define CFG_HANDLING 0x0cu
#define CFG_TRANS 0x10u
#define CFG_STAGE 0x1cu
#define CFG_AREA 0x24u
#define CFG_DIST 0x28u
#define CFG_GOAL 0x2cu           /* the goal's odometer reading (SRaceField +0x10) */
#define CFG_MODE 0x34u
#define CFG_CARD 0x19u           /* a byte: the player has a card (clV386CardData::hasCard) */
#define CFG_LEVEL 0x38u          /* the story's loop (initStory: clWorkSpace +0x20, "lap") */
#define CFG_LOSSES 0x3cu         /* the episode's losses (clWorkSpace +0x24), easing the rivals */
#define CFG_RIVALS 0x44u         /* how many rivals (up to 3), then their ECharacter and ECarType */
#define CFG_RIVAL_CHAR 0x48u
#define CFG_RIVAL_CAR 0x54u
#define CFG_SERIES 0x40u         /* the story's ESeries (CSeq::Init: CConfigLoader::GetStorySeries) */
#define STORY_CONFIG RCA(9433da8)  /* "data/carconfig/ytConfigStory.txt" */
#define RACE_FIELD 0x1cu         /* SConfigRaceField, 6 words: CFG_STAGE .. the goal and on */
#define CFG_TA_TRAFFIC 0xacu     /* clWorkSpace::isTaOtherCar: time attack with the traffic on */
#define CFG_TEN_LEVEL 0xb8u      /* the 10-in-a-row's level (clRaceThread::initTen: 10 = extend) */
#define MODE_TEN_FIGHT 7u        /* EGameMode of the 10-in-a-row */
/* CSeq's globals */
#define MACHINE_PTR RCA(98028e4)   /* CMachine */
#define OTHER_GEN_PTR RCA(9802894) /* COtherGen, the traffic */
#define LX_RIVALS RCA(980289c)     /* yt::CLxRival *[3]: a versus' link cars (rivals 1..3) */
#define COUNTDOWN RCA(9802a68)     /* CState: seconds of countdown left (Prep_CountDown sets 4) */
#define TIME_KEEPER_PTR RCA(98028c8) /* CTimeKeeper, the checkpoints (the first is the start, the last the goal) */
#define TK_COUNT 0x00u           /* checkpoints */
#define TK_ODO 0x04u             /* float[]: the odometer reading of each (the last is the goal) */
#define TK_EXTEND 0x08u          /* float[]: seconds each one gives */
#define TK_SPLIT 0x10u           /* float[]: seconds since the start when each was passed */
#define TK_NEXT 0x14u            /* the next one to pass */
#define PLAY_VIEW RCA(9802820)     /* CSeq's EPlayView (CSeq::GetInfoCarCamera hands it CMachine's) */
#define STATE RCA(9802a78)         /* CState::GetStateName: 2 Run, 4 SlowDown, 5 StopWait, 6 TimeUp, 7 GoalWait, 8 Goal */
/* CSeq::Init gets the STireDimension from the display model here: a hook hands it the spec's */
#define TIRE_HOOK RCA(898aa8d)
#define TIRE_FRAME 0xd4u         /* the STireDimension at ebp - 0xd4 */
#define TEN_TIRE_HOOK RCA(8985a4a)   /* CSeq::Exec's next 10-in-a-row rival: CRivalSetting::GetSettingPt's 4th argument */
#define CUTIN_ON RCA(9802970)      /* yt::CSeq::GetCutInInfoPt: the info at *CUTIN_INFO while set */
#define CUTIN_INFO RCA(9802860)
#define TEN_RIVAL_NO RCA(9802acc)    /* CTenSeq: the rival on the road now, 0 the first */
#define RIVAL_TIRE_HOOK RCA(898e699) /* and CRivalSetting::GetSettingPt the rival's, its 6th argument */
#define COMPENSATE_HOOK RCA(8984557) /* CSeq::Exec right after CLinx::Compensate */
/* CMachine fields */
#define M_ENGINE 0x2c4u          /* CTransmission (engine state at +8: rad/s, AT flag at +0x34, gear at +0x3c) */
#define M_TURBO 0x340u           /* CTurbo (boost at +4) */
#define M_AREA_PROC 0x938u       /* CAreaProcessorBase*, followed by its SAreaProcessorInfo */
#define JCT_BLOCK_NAMES RCA(93d9d00) /* const char* by junction block id (clCourseArrow) */
#define TA_LENGTHS RCA(9778900)    /* GetTimeAttackRaceField: by EStage from 363, {u32 start, f32 length in mm} */
#define TA_STAGE0 363u

/* The time attack courses by course card, with the start each one gets from
 * GetConfigRaceField_TimeAttack(EStage): area and distance; and the arcade's course id
 * (nsMenuCheck::taCourseIDFromMenuOrder of the card's place: ids 5, 7 and 11 have no card). */
static const struct { uint32_t stage, area; float dist; int id; } COURSES[CAR_COURSES] = {
  { 363, 28, 1535000.0f, 0 },   /* TA_C1In: TkC1_In */
  { 364, 36, 2930000.0f, 1 },   /* TA_C1Out: TkGinza_Down */
  { 365, 44, 1095000.0f, 2 },   /* TA_RingLeft: TkN9_In */
  { 366, 37, 670000.0f, 3 },    /* TA_RingRight: TkGinza_Up */
  { 367, 45, 7220000.0f, 4 },   /* TA_WanganEast: TkN9_Out */
  { 369, 13, 1185000.0f, 6 },   /* TA_K1_Down: KgK1_Out */
  { 376, 64, 150000.0f, 8 },    /* NAGOYA: NkNagoya */
  { 371, 5, 5000.0f, 9 },       /* OSAKA: OsTonbori */
  { 377, 72, 11870000.0f, 14 }, /* FUKUOKA: FkFukuoka */
  { 372, 59, 390000.0f, 10 },   /* HAKONE_OUTWARD: HkHakone */
  { 374, 55, 3630000.0f, 12 },  /* SYUTOKO_TOKYO: TkTennouzu_Up */
  { 375, 56, 815000.0f, 13 },   /* SYUTOKO_KANAGAWA: TkTokyokou_Down */
};

int car_course_id(int course) { return course >= 0 && course < CAR_COURSES ? COURSES[course].id : 0; }
int car_course_ta_slot(int course) {
  return course >= 0 && course < CAR_COURSES && COURSES[course].stage - TA_STAGE0 < 15u ? (int)(COURSES[course].stage - TA_STAGE0) : -1;
}

/* EAreaId by city (the area names' prefixes): Os 1..9, Hk 58..61, Nk 62..69, Fk 70..73 */
int car_area_city(int area) {
  if (area >= 1 && area <= 9) return CAR_OSAKA;
  if (area >= 58 && area <= 61) return CAR_HAKONE;
  if (area >= 62 && area <= 69) return CAR_NAGOYA;
  if (area >= 70 && area <= 73) return CAR_FUKUOKA;
  return CAR_TOKYO;
}

int car_course_city(int course) { return course >= 0 && course < CAR_COURSES ? car_area_city((int)COURSES[course].area) : CAR_TOKYO; }

const car_spec CAR_SPEC_FD3S = { 5u, 5, { 690.0f, 298.3f, 1285.0f, 684.9f, 298.3f, -1065.2f }, 5, 24, 10 };

int car_tune_hp(int stock_hp, int power) {
  /* the table at 0x9736a60 (gauge_pow_to_Horse's too) */
  static const short HP[CAR_TUNE_MAX + 1] = { 0,   360, 400, 420, 450, 470, 500, 530, 550, 580, 600, 620, 640,
                                              660, 680, 700, 720, 740, 760, 780, 800, 810, 815, 820, 825, 850, 875, 910, 925, 950, 975, 1000 };
  if (power <= 0) return stock_hp;
  return HP[power > CAR_TUNE_MAX ? CAR_TUNE_MAX : power];
}

int car_tune_grade(int power, int handling) {
  int d = handling - power;
  return d <= -8 ? CAR_GRADE_DG : d <= -4 ? CAR_GRADE_D : d <= 3 ? CAR_GRADE_B : d <= 7 ? CAR_GRADE_G : CAR_GRADE_HG;
}

/* the traffic's ECarType (42..45) to its model */
static const int TRAFFIC_MODEL[4] = { CAR_TRAFFIC_RC2, CAR_TRAFFIC_NZE121, CAR_TRAFFIC_KZH100G, CAR_TRAFFIC_TRUCK };
#define TRAFFIC_TYPE0 42u
#define TRAFFIC_SLOT0 4 /* COtherGen's cars are 4..15 (0..3 the player and the rivals) */

static struct {
  char image[1024], config[1024];
  int course;
  car_spec spec;
  uint32_t m, gen, gin, gin_ptr, disp, eff, cam, mirror, jct, info_race, info_map, info_sign, info_jct, jct_line, lines, ten_line;
  float jct_saved[3][3];
  int jct_prev;
  int view;
  int manual, lever, top_lever; /* lever 0..gears-1, -1 = neutral */
  int traffic_model[CAR_TRAFFIC];
  int has_rival, n_rival;
  int ten_stage, ten_level; /* car_set_ten: the 10-in-a-row's EStage (0: not one) and level */
  int n_ten;
  uint32_t info_ten;
  car_rival ten[CAR_TEN_RIVALS];
  car_rival rival[CAR_RIVALS];
  /* car_set_versus: the link race's machines (vs_n others), ramp, km and seed */
  int vs_on, vs_pcb, vs_n, vs_ramp;
  int ta_traffic; /* car_set_ta_traffic */
  int vs_race; /* the link's cars in the race now (0: not a versus) */
  float vs_km;
  unsigned vs_seed;
  car_link_car vs[CAR_LINK_MACHINES - 1];
  car_link_io vs_io;
  uint32_t info_story, result, first_pt, title_log;
  int ready;
} P = { .view = CAR_VIEW_BEHIND };

static uint32_t fbits(float v) { uint32_t u; memcpy(&u, &v, 4); return u; }

/* cdecl call into recompiled code, with arguments on the emulated stack */
static uint32_t call(void (*fn)(void), int n, ...) {
  uint32_t a[10];
  va_list ap;
  va_start(ap, n);
  for (int i = 0; i < n; i++) a[i] = va_arg(ap, uint32_t);
  va_end(ap);
  esp = RC_STACK_TOP - 0x1000 - 4 * n - 4;
  W32(esp, 0x20200000u);
  for (int i = 0; i < n; i++) W32(esp + 4 + 4 * i, a[i]);
  fn();
  return eax;
}

int car_init(const char *data_dir, char *err, int err_len) {
  snprintf(P.image, sizeof P.image, "%s/phys.img", data_dir);
  /* yt::CConfigLoader's files: data_dir/carconfig (the story's rivals, the 10-in-a-row) */
  snprintf(P.config, sizeof P.config, "%s", data_dir);
  rc_config_dir = P.config;
  FILE *f = fopen(P.image, "rb");
  if (!f) {
    snprintf(err, err_len, "cannot load %s", P.image);
    return -1;
  }
  fclose(f);
  return 0;
}

/* CSeq::Init: STireDimension, normally from the display model: front (x, radius, z), rear, spokes; mm */
static void lx_override(void);
static void on_hook(uint32_t at) {
  if (at == COMPENSATE_HOOK) {
    lx_override();
    return;
  }
  if (at == RIVAL_TIRE_HOOK) { /* CSeq::Init's loop over the rivals: edi the rival's number */
    const car_rival *r = &P.rival[edi >= 1 && edi <= (uint32_t)P.n_rival ? edi - 1 : 0];
    uint32_t tire = M32(esp + 0x14);
    for (int i = 0; i < 6; i++) WF(tire + 4 * i, r->tyres[i]);
    W32(tire + 0x18, (uint32_t)r->spokes);
    return;
  }
  if (at == TEN_TIRE_HOOK) {
    uint32_t k = M32(TEN_RIVAL_NO);
    if (k >= (uint32_t)P.n_ten) return;
    uint32_t tire = M32(esp + 0xc);
    for (int i = 0; i < 6; i++) WF(tire + 4 * i, P.ten[k].tyres[i]);
    W32(tire + 0x18, (uint32_t)P.ten[k].spokes);
    return;
  }
  if (at != TIRE_HOOK) return;
  uint32_t tire = ebp - TIRE_FRAME;
  for (int i = 0; i < 6; i++) WF(tire + 4 * i, P.spec.tyres[i]);
  W32(tire + 0x18, (uint32_t)P.spec.spokes);
  /* the cars after it, 0x1c apart (ebp-0xb8, -0x9c, -0x80): yt::GetTireDimension's for cars 1..3
   * (clCarDispManager::getRivalTireParam, the display side's). The realtime movies' ghosts take
   * them (the loop at 0x898bc15: CGhostSetting::GetSettingPt with car i's); unset, their tyres
   * stood nowhere (NaN) */
  for (int k = 0; k < P.n_rival && k < 3; ++k) {
    uint32_t t = tire + 0x1cu * (uint32_t)(k + 1);
    for (int i = 0; i < 6; i++) WF(t + 4 * i, P.rival[k].tyres[i]);
    W32(t + 0x18, (uint32_t)P.rival[k].spokes);
  }
  /* in a versus the link's cars (CSeq::Init's loop at 0x898e800: yt::CLxRivalSetting::GetSettingPt
   * with car k + 1's) */
  for (int k = 0; P.vs_on && k < P.vs_n; ++k) {
    uint32_t t = tire + 0x1cu * (uint32_t)(k + 1);
    for (int i = 0; i < 6; i++) WF(t + 4 * i, P.vs[k].tyres[i]);
    W32(t + 0x18, (uint32_t)P.vs[k].spokes);
  }
}

void car_set_ta_traffic(int on) { P.ta_traffic = on != 0; }

void car_set_rivals(const car_rival *rivals, int n) {
  P.n_rival = n < 0 ? 0 : n > CAR_RIVALS ? CAR_RIVALS : n;
  P.has_rival = P.n_rival > 0;
  for (int i = 0; i < P.n_rival; ++i) P.rival[i] = rivals[i];
}

void car_set_ten(int stage, int level, const car_rival *rivals, int n) {
  P.ten_stage = stage;
  P.ten_level = level;
  P.n_ten = n < 0 ? 0 : n > CAR_TEN_RIVALS ? CAR_TEN_RIVALS : n;
  for (int i = 0; i < P.n_ten; ++i) P.ten[i] = rivals[i];
}

/* The versus race. clRaceThread::initJoin fills SConfigPlay's link part: +0x60 the join's
 * NoRubberBand, +0x64 this machine's place among the joined (its PCB id's), +0x68 how many others,
 * then per other machine i: +0x6c its PCB id, +0x78 its place, +0x84 its ECarType (from its card),
 * +0x90 / +0x9c its power and handling tune, and +0xa8 the race's random number; the start from
 * GetConfigRaceField_Battle(ERamp of the join ramp: table 0x9739140, +8) and the goal its distance
 * on (clNet::getJoinDist km). CSeq::Init then has CLinx::Init(+0x64, +0x68, +0x6c, +0x78). */
#define MODE_BATTLE 5u
#define CFG_LINK_NO_BAND 0x60u
#define CFG_LINK_PLACE 0x64u
#define CFG_LINK_OTHERS 0x68u
#define CFG_LINK_PCB 0x6cu
#define CFG_LINK_OTHER_PLACE 0x78u
#define CFG_LINK_CAR 0x84u
#define CFG_LINK_POWER 0x90u
#define CFG_LINK_HANDLING 0x9cu
#define CFG_LINK_RANDOM 0xa8u
#define JOIN_RAMPS RCA(9739140) /* the join ramps: {name, course, ERamp, ...} */
#define LINK_BUF 0x400u       /* clNet's race packet buffers */
void f_8992604(void); /* yt::GetBattleRaceField(ERamp) */

static void lx_reset(void); /* (below: the measuring of the arcade's latency compensation) */
static void rx_reset(void);
static void sp_reset(void);
/* The pilot drives while a versus race is paused (the others race on: the arcade's CMcPilot steers and the speed the car had is held, until the pause is left) */
static int AUTOP;
static float AUTOP_V;
static float autop_speed; /* (car_update's last speed, m/s) */
void car_autopilot(int on) {
  if (on && !AUTOP) AUTOP_V = fmaxf(autop_speed, 15.0f), fprintf(stderr, "versus: paused, the pilot drives at %.0f km/h\n", (double)AUTOP_V * 3.6);
  if (!on && AUTOP) fprintf(stderr, "versus: the pause is left, the car is back in hand at %.0f km/h\n", (double)autop_speed * 3.6);
  AUTOP = on != 0;
}
/* A versus' other cars' crashes: their packets carry no hit, so a car whose speed stays 8 m/s or more under what it was (the highest of 12 steps before the last 8) is a crash for
 * the sound (a traffic car beside you, a wall; the arcade plays it for every car near you). A stall of the packets (a few steps of no motion, then a jump) is not one: the last 8 steps
 * must all be slow. Once a second at most each. */
static struct { float pos[3]; float spd[20]; int n, cool; } RH[CAR_RIVALS];
static void rival_impacts(car_state *c) {
  for (int k = 0; k < CAR_RIVALS; ++k) {
    c->rival_impact[k] = 0.0f;
    if (!P.vs_race || !c->rival_shown[k]) {
      RH[k].n = 0;
      continue;
    }
    const float *b = c->rival_body[k] + 12;
    float spd = 0.0f;
    if (RH[k].n > 0) {
      const float dx = b[0] - RH[k].pos[0], dy = b[1] - RH[k].pos[1], dz = b[2] - RH[k].pos[2];
      spd = sqrtf(dx * dx + dy * dy + dz * dz) * 6.0f; /* (units of 0.1 m a step) */
      if (spd > 150.0f) RH[k].n = 0, spd = 0.0f; /* (a warp, not a drive) */
    }
    memcpy(RH[k].pos, b, sizeof RH[k].pos);
    if (RH[k].cool > 0) --RH[k].cool;
    if (RH[k].n == 0) {
      RH[k].n = 1;
      continue;
    }
    if (RH[k].n < 20) {
      RH[k].spd[RH[k].n++ - 1] = spd;
      continue;
    }
    for (int i = 0; i < 18; ++i) RH[k].spd[i] = RH[k].spd[i + 1];
    RH[k].spd[18] = spd;
    float top = 0.0f, recent = 0.0f;
    for (int i = 0; i < 11; ++i) top = fmaxf(top, RH[k].spd[i]);
    for (int i = 11; i < 19; ++i) recent = fmaxf(recent, RH[k].spd[i]);
    if (top - recent >= 8.0f && RH[k].cool == 0) {
      c->rival_impact[k] = top - recent;
      RH[k].cool = 90;
      fprintf(stderr, "versus: car %d lost %.1f m/s (%.0f to %.0f km/h): a crash, for the sound, race time %.2f\n", k, (double)(top - recent), (double)(top * 3.6f), (double)(recent * 3.6f), (double)c->race.total);
    }
  }
}
static float vs_time_km; /* this race's km if it is a versus (the time it starts with, first_frame) */
static unsigned link_gone;
static uint32_t gone_cnt[CAR_LINK_MACHINES];
void car_set_versus(int pcb, const car_link_car *others, int n, int ramp, float km, unsigned seed, const car_link_io *io) {
  link_gone = 0;
  lx_reset();
  P.vs_on = n > 0 && io;
  P.vs_pcb = pcb;
  P.vs_n = n < 0 ? 0 : n > CAR_LINK_MACHINES - 1 ? CAR_LINK_MACHINES - 1 : n;
  for (int i = 0; i < P.vs_n; ++i) P.vs[i] = others[i];
  P.vs_ramp = ramp < 0 || ramp >= CAR_LINK_RAMPS ? 0 : ramp;
  P.vs_km = km;
  P.vs_seed = seed;
  if (io) P.vs_io = *io;
}

const char *car_link_ramp_name(int ramp) {
  if (ramp < 0 || ramp >= CAR_LINK_RAMPS || !P.ready) return "";
  return (const char *)VA(M32(M32(JOIN_RAMPS + 4u * (uint32_t)ramp)));
}

/* clNet::receiveRacePacket / sendRacePacket, the port's */
static void link_gone_step(void);
static void link_receive(void) {
  if (P.vs_io.receive) P.vs_io.receive(P.vs_io.ctx);
  link_gone_step();
}

void car_link_drop(int pcb) {
  if (pcb >= 0 && pcb < CAR_LINK_MACHINES) link_gone |= 1u << pcb, gone_cnt[pcb] = 0;
}
static void link_send(void) { if (P.vs_io.send) P.vs_io.send(VA(rc_link_tx), CAR_LINK_PACKET, P.vs_io.ctx); }

/* ---- measuring the arcade's own latency compensation. CLinx::Compensate (run each frame after the packets are taken in) pushes each other machine's
 * packet forward by its estimate of the delay: position += (velocity + acceleration * t / 2) * t, from the SLxPacketRival at +0x210 of the packet
 * (position +0x10, velocity +0x48, acceleration +0x54 in it), and the rival's car and its hit are made from the result. Here: where that put the other
 * machine at this machine's frame n, against where it really was at its frame n (the packet that comes with count n, a few frames later), and the same
 * for the packet's position as it is and for the straight formula with the real age. Logged every 2 s of a race. */
#define LX_PCB_ARRAY RCA(98033e0)
#define LX_PCB_STEP 0x2bcu
#define LX_NCNT RCA(98033a0)
#define LX_POS 0x220u /* in the packet and in CLinx::LxPcb's copy of it */
#define LX_VEL 0x258u
#define LX_ACC 0x264u
#define LX_COUNT 0x34u

/* a machine that left: its packets stand in the buffer parked far out of reach, the counter running on (no car on the road, nothing to hit) */
static void link_gone_step(void) {
  for (int k = 0; link_gone && k < CAR_LINK_MACHINES; ++k) {
    if (!(link_gone >> k & 1u) || !rc_link_rx) continue;
    uint32_t rp = rc_link_rx + LINK_BUF * (uint32_t)k;
    if (!gone_cnt[k]) gone_cnt[k] = M32(rp + LX_COUNT);
    W32(rp + LX_COUNT, ++gone_cnt[k]);
    static const float far_away[3] = { 0.0f, -2.0e7f, 0.0f }, still[3] = { 0, 0, 0 };
    memcpy(VA(rp + LX_POS), far_away, sizeof far_away);
    memcpy(VA(rp + LX_VEL), still, sizeof still);
    W8(rp + 0x74u, 1); /* (and finished: CState::Ck_GoalWait_2_Goal waits for every machine's flag at +0x74 of its packet, which the one that left never sets) */
    for (int i = 0; i < CAR_LINK_MACHINES; ++i) { /* (GetRecv() is the CLxPcb entry itself, which entry by its +0x284 pcb: the game reads the flag there) */
      uint32_t e = LX_PCB_ARRAY + LX_PCB_STEP * (uint32_t)i;
      if ((int)M32(e + 0x284u) == k) W8(e + 0x74u, 1);
    }
  }
}
typedef struct {
  float comp[3], raw[3], lin[3];
  uint32_t cnt;
  int ok;
} lx_rec;
static lx_rec lx_ring[CAR_LINK_MACHINES][256];
static uint32_t lx_last[CAR_LINK_MACHINES];
static struct {
  double comp, raw, lin, comp_max;
  int n;
} lx_err[CAR_LINK_MACHINES];
static int lx_frames, lx_dbg_rec, lx_dbg_arr, lx_dbg_match, lx_dbg_skip[4];

static void lx_reset(void) {
  sp_reset();
  rx_reset();
  memset(lx_ring, 0, sizeof lx_ring);
  memset(lx_last, 0, sizeof lx_last);
  memset(lx_err, 0, sizeof lx_err);
  lx_frames = 0;
}

/* Right after CLinx::Compensate: each other machine's position (and odometer) put where it is NOW by the packet's age (this machine's frame
 * counter less the packet's: the clocks are locked to the host's, so that is the delay), from the packet as it came (rc_link_rx), not by Compensate's
 * own estimate of the delay (half the round trip from acks, which measured 1.5 m out at 70 m/s where this is 4 cm out). The velocity and acceleration
 * stay the packet's: the hit uses them. */
static int lx_ovr_n, lx_skipped; /* (the packets Compensate was left to: too old, or from too far ahead) */
/* the other cars' angles (CEuler: three 16-bit angles, 65536 to a turn): their rate per frame from the packets (sp_look), and the prediction of the turn by the
 * packet's age (lx_override); MT3DX_ANG=0 turns the prediction off */
#define AN_RATE_MAX 4096.0f /* 22 degrees a frame */
#define AN_DAMP 0.8f
#define AN_MAX 8192.0f /* the most a prediction turns an angle, 45 degrees */
static int16_t an_prev[CAR_LINK_MACHINES][3];
static uint32_t an_prev_cnt[CAR_LINK_MACHINES];
static float an_rate[CAR_LINK_MACHINES][3];
static int an_ok[CAR_LINK_MACHINES], an_on = 1;
static uint32_t lx_hook_cnt; /* the frame counter as the hook saw it (it ticks after: the scoring keys by this one) */
static uint32_t lx_contact_cnt = 0xffffffffu; /* the frame of the last contact (car_link_contact) */
static struct {
  double sum, max;
  int n;
} lx_bk[4]; /* the used prediction's error by the frames since a contact: 0-5, 6-15, 16-30, 31-120 */
/* ---- the other car's reaction to a contact (Newton's third law on the prediction). A contact is decided on each machine against the prediction of the
 * other car, and each machine's own car gets its impulse; the other car's own machine does the same for it, but that reaction reaches here a packet's
 * age later, and until then the prediction runs on straight (half a metre out for the first frames, metres at worst: the bounce). So when this car's
 * velocity changes suddenly with the other close, and away from it, the other car is given the equal and opposite change (equal masses) in the
 * prediction, from that frame on, for as long as the packets in hand were sent before it (the first sent after carry the other machine's own reaction).
 * MT3DX_REACT=<factor> scales it (default 1, 0 off). */
#define RX_MAX 16
#define RX_DV_MIN 500.0f   /* mm/s a frame: the change of velocity of a hit (0.5 m/s, as car.c's cues) */
#define RX_NEAR_MM 4200.0f /* the centres' distance within which a change of velocity is this car's: the cars are 4.5 m long */
#define RX_DV_MAX 6000.0f  /* a change of velocity past this (6 m/s a frame) is a wall or a crash, not a touch: no reaction is made of it */
#define RX_POS_MAX 1500.0f /* the most the reactions move the prediction, mm */
#define RX_START_FRAMES 600 /* the grid, the countdown and the launch: the cars touch there and the reaction is left out */
#define RX_BLEND 4 /* frames over which the given reaction fades as the packets with the real one arrive */
typedef struct {
  uint32_t f; /* the frame of the contact (the hook's counter) */
  float dx, dz; /* the other car's change of velocity, mm/s */
} rx_contact;
static rx_contact rx_list[CAR_LINK_MACHINES][RX_MAX];
static int rx_n[CAR_LINK_MACHINES];
static float rx_factor = 1.0f, rx_pm[3], rx_pv[3];
static int rx_have_pm, rx_have_pv, rx_logged;

static void rx_reset(void) {
  memset(rx_list, 0, sizeof rx_list);
  memset(rx_n, 0, sizeof rx_n);
  rx_have_pm = rx_have_pv = rx_logged = 0;
  const char *e = getenv("MT3DX_REACT");
  rx_factor = e ? (float)atof(e) : 1.0f;
  e = getenv("MT3DX_ANG");
  an_on = e ? atoi(e) != 0 : 1;
}

/* after the arcade frame: this car's velocity from its position, and whether it changed like a hit with a close car */
static void rx_detect(void) {
  float pm[3] = { MF(P.m + 0x408u), MF(P.m + 0x40cu), MF(P.m + 0x410u) };
  int ok = rx_have_pm;
  float v[3] = { (pm[0] - rx_pm[0]) * 60.0f, (pm[1] - rx_pm[1]) * 60.0f, (pm[2] - rx_pm[2]) * 60.0f };
  if (ok && rx_have_pv && lx_hook_cnt > RX_START_FRAMES) { /* this car moved by more than its velocity gives: the game pushed it (a hit's separation), not driving */
    float jx = pm[0] - (rx_pm[0] + rx_pv[0] * (1.0f / 60.0f)), jz = pm[2] - (rx_pm[2] + rx_pv[2] * (1.0f / 60.0f)), jl = sqrtf(jx * jx + jz * jz);
    static int jumps, logged;
    if (jl > 150.0f) {
      float nearest = 1e9f;
      for (int i = 0; i < CAR_LINK_MACHINES; ++i) {
        uint32_t e = LX_PCB_ARRAY + LX_PCB_STEP * (uint32_t)i;
        int pcb = (int)M32(e + 0x284);
        if (pcb < 0 || pcb >= CAR_LINK_MACHINES || !VA(e + 0x290)[0]) continue;
        float ox = pm[0] - MF(e + LX_POS), oz = pm[2] - MF(e + LX_POS + 8), d = sqrtf(ox * ox + oz * oz);
        if (d < nearest) nearest = d;
      }
      jumps++;
      if (logged < 50) {
        logged++;
        NETLOG("versus own jump: this car moved %.2f m past its velocity (the %d-th), the other car %.1f m away, %d frames after my last contact\n", (double)jl * 0.001, jumps, nearest < 1e8f ? (double)nearest * 0.001 : -1.0,
                lx_contact_cnt == 0xffffffffu ? -1 : (int)(int32_t)(lx_hook_cnt - lx_contact_cnt));
      }
    }
  }
  memcpy(rx_pm, pm, sizeof pm);
  rx_have_pm = 1;
  if (!ok) return;
  float dvx = v[0] - rx_pv[0], dvz = v[2] - rx_pv[2], dv = sqrtf(dvx * dvx + dvz * dvz);
  int had = rx_have_pv;
  memcpy(rx_pv, v, sizeof v);
  rx_have_pv = 1;
  if (!had || rx_factor <= 0.0f || dv < RX_DV_MIN || dv > RX_DV_MAX || lx_hook_cnt < RX_START_FRAMES) return;
  for (int i = 0; i < CAR_LINK_MACHINES; ++i) {
    uint32_t e = LX_PCB_ARRAY + LX_PCB_STEP * (uint32_t)i;
    int pcb = (int)M32(e + 0x284);
    if (pcb < 0 || pcb >= CAR_LINK_MACHINES || !VA(e + 0x290)[0]) continue;
    float ox = pm[0] - MF(e + LX_POS), oz = pm[2] - MF(e + LX_POS + 8), d = sqrtf(ox * ox + oz * oz);
    if (d > RX_NEAR_MM || d < 1.0f) continue;
    if (dvx * ox + dvz * oz < 0.3f * dv * d) continue; /* (pushed towards it, or sideways: a wall's, not its) */
    if (rx_n[pcb] >= RX_MAX) continue;
    float k = rx_factor;
    rx_contact *c = &rx_list[pcb][rx_n[pcb]++];
    c->f = lx_hook_cnt, c->dx = -dvx * k, c->dz = -dvz * k;
    if (rx_logged++ < 40) NETLOG("versus react: contact at frame %u with pcb %d, %.1f m away: my change of velocity %.2f %.2f m/s, its %.2f %.2f m/s\n", c->f, pcb, (double)d * 0.001, (double)dvx * 0.001, (double)dvz * 0.001, (double)c->dx * 0.001, (double)c->dz * 0.001);
  }
}

/* in the override: this machine's contacts the packets in hand (count c <= the frame) were sent before, added to the prediction: the velocity from then,
 * the position by what it has gone on since; the older ones dropped */
static void rx_apply(int pcb, uint32_t e, uint32_t rp, uint32_t count, uint32_t ncnt) {
  float sx = 0, sz = 0, px = 0, pz = 0;
  int w = 0;
  for (int k = 0; k < rx_n[pcb]; ++k) {
    rx_contact c = rx_list[pcb][k];
    int32_t since = (int32_t)(ncnt - c.f);
    int32_t sent_after = (int32_t)(count - c.f); /* (frames the packet in hand was sent after the contact: from the first, it carries the other machine's own reaction) */
    if (since > 240 || sent_after >= RX_BLEND) continue;
    rx_list[pcb][w++] = c;
    if (since < 0) continue;
    float wt = sent_after <= 0 ? 1.0f : 1.0f - (float)sent_after / (float)RX_BLEND; /* (handed over to the real one over a few frames, not at once) */
    sx += c.dx * wt, sz += c.dz * wt;
    px += c.dx * wt * (float)since * (1.0f / 60.0f), pz += c.dz * wt * (float)since * (1.0f / 60.0f);
  }
  rx_n[pcb] = w;
  (void)sx, (void)sz;
  /* the position only (the velocity the hit works from stays the packet's: handing it a sum of several frames' changes fed itself, each hit making the
   * next one harder, and one run's car spun up to hundreds of thousands of degrees), and what is added is capped */
  float len = sqrtf(px * px + pz * pz);
  if (len > RX_POS_MAX) px *= RX_POS_MAX / len, pz *= RX_POS_MAX / len;
  WF(e + LX_POS, MF(e + LX_POS) + px);
  WF(e + LX_POS + 8, MF(e + LX_POS + 8) + pz);
}

static void lx_override(void) {
  uint32_t ncnt = M32(LX_NCNT);
  lx_hook_cnt = ncnt;
  for (int i = 0; i < CAR_LINK_MACHINES; ++i) {
    uint32_t e = LX_PCB_ARRAY + LX_PCB_STEP * (uint32_t)i;
    int pcb = (int)M32(e + 0x284);
    if (pcb < 0 || pcb >= CAR_LINK_MACHINES || !VA(e + 0x290)[0]) continue;
    uint32_t rp = rc_link_rx + LINK_BUF * (uint32_t)pcb, count = M32(rp + LX_COUNT);
    float t = (float)(int32_t)(ncnt - count) * (1.0f / 60.0f);
    /* a packet from a machine whose frame counter is ahead of this one's (the clocks are only held to within a few frames of the host's, so two clients differ by that
     * and a jittery link swings it) has a negative age: its car is put back along its velocity, not left to Compensate's own estimate, which is metres out (a rival
     * that jumped back and forth, 4-way versus). MT3DX_NEGAGE=0 restores the old skip, for a comparison */
    static int neg_ok = -1;
    if (neg_ok < 0) neg_ok = getenv("MT3DX_NEGAGE") ? atoi(getenv("MT3DX_NEGAGE")) != 0 : 1;
    /* (up to a second old: past half a second Compensate was left to, and its car stood still then jumped a dozen metres when the packets came back after a stall) */
    if (!count || t > 1.0f || t < (neg_ok ? -0.25f : 0.0f)) {
      lx_skipped++;
      continue;
    }
    if (lx_ovr_n < 3 || (lx_ovr_n % 600) == 0) NETLOG("versus predict: override pcb %d count %u ncnt %u age %.1f ms: Compensate left %.0f %.0f %.0f, packet %.0f %.0f %.0f, vel %.0f %.0f %.0f\n", pcb, count, ncnt, (double)t * 1000.0, (double)MF(e + LX_POS), (double)MF(e + LX_POS + 4), (double)MF(e + LX_POS + 8), (double)MF(rp + LX_POS), (double)MF(rp + LX_POS + 4), (double)MF(rp + LX_POS + 8), (double)MF(rp + LX_VEL), (double)MF(rp + LX_VEL + 4), (double)MF(rp + LX_VEL + 8));
    lx_ovr_n++;
    { /* the position; a new packet after a gap (a stall, a lost run of them) puts the car where it really is, metres from where the last ones had it going: that is shown
       * as a car that jumps and comes back. So the position goes on from last step's, by the packet's velocity, and is drawn toward the packet's prediction by a fifth of what
       * is between them each step (a jump of a metre is gone in about a tenth of a second; past SM_SNAP, or a step that is not the next one, it is a teleport and not smoothed).
       * MT3DX_SMOOTH=0 turns it off, for a comparison. */
      static float sm_out[CAR_LINK_MACHINES][3];
      static uint32_t sm_cnt[CAR_LINK_MACHINES];
      static int sm_on = -1;
      if (sm_on < 0) sm_on = getenv("MT3DX_SMOOTH") ? atoi(getenv("MT3DX_SMOOTH")) != 0 : 1;
      float raw[3], out[3], err2 = 0.0f;
      const int next = sm_on && sm_cnt[pcb] == ncnt - 1u;
      for (int j = 0; j < 3; ++j) {
        const float v = MF(rp + LX_VEL + 4u * (uint32_t)j);
        const float ta = t > 0.25f ? 0.25f : t; /* (the acceleration is carried on for a quarter of a second at most: a stale packet's would run away) */
        raw[j] = MF(rp + LX_POS + 4u * (uint32_t)j) + v * t + MF(rp + LX_ACC + 4u * (uint32_t)j) * (0.5f * ta * ta + ta * (t - ta));
        const float cont = sm_out[pcb][j] + v * (1.0f / 60.0f); /* (where it would be had last step's gone on) */
        out[j] = next ? cont + 0.2f * (raw[j] - cont) : raw[j];
        err2 += (raw[j] - cont) * (raw[j] - cont);
      }
      const int snap = !next || err2 > 6000.0f * 6000.0f; /* (6 m) */
      for (int j = 0; j < 3; ++j) {
        if (snap) out[j] = raw[j];
        sm_out[pcb][j] = out[j];
        WF(e + LX_POS + 4u * (uint32_t)j, out[j]);
      }
      sm_cnt[pcb] = ncnt;
    }
    WF(e + 0x240u,MF(rp + 0x240u) + MF(rp + 0x244u) * t); /* (the odometer: the speed times it) */
    rx_apply(pcb, e, rp, count, ncnt);
    if (an_on && an_ok[pcb]) { /* the angles turned on by the packet's age at the rate they were turning */
      float age = (float)(int32_t)(ncnt - count);
      for (int j = 0; j < 3; ++j) {
        int16_t raw;
        memcpy(&raw, VA(rp + 0x210u + 2u * (uint32_t)j), 2);
        float adv = an_rate[pcb][j] * age * AN_DAMP;
        adv = adv > AN_MAX ? AN_MAX : adv < -AN_MAX ? -AN_MAX : adv;
        int16_t pred = (int16_t)(raw + (int)lroundf(adv));
        memcpy(VA(e + 0x210u + 2u * (uint32_t)j), &pred, 2);
      }
    }
  }
}

/* after the arcade frame: what Compensate made of each other machine, and what the packet it started from said */
static void lx_record(void) {
  rx_detect();
  uint32_t ncnt = lx_hook_cnt;
  for (int i = 0; i < CAR_LINK_MACHINES; ++i) {
    uint32_t e = LX_PCB_ARRAY + LX_PCB_STEP * (uint32_t)i;
    int pcb = (int)M32(e + 0x284);
    if (pcb < 0 || pcb >= CAR_LINK_MACHINES || !VA(e + 0x290)[0]) continue;
    uint32_t rp = rc_link_rx + LINK_BUF * (uint32_t)pcb;
    uint32_t count = M32(rp + LX_COUNT);
    float t = (float)(int32_t)(ncnt - count) * (1.0f / 60.0f);
    if (t < -0.25f || t > 1.0f) {
      lx_dbg_skip[i]++;
      continue;
    }
    lx_dbg_rec++;
    lx_rec *r = &lx_ring[pcb][ncnt & 255];
    for (int j = 0; j < 3; ++j) {
      r->comp[j] = MF(e + LX_POS + 4u * (uint32_t)j);
      r->raw[j] = MF(rp + LX_POS + 4u * (uint32_t)j);
      r->lin[j] = r->raw[j] + (MF(rp + LX_VEL + 4u * (uint32_t)j) + 0.5f * MF(rp + LX_ACC + 4u * (uint32_t)j) * t) * t;
    }
    r->cnt = ncnt, r->ok = 1;
  }
  if (++lx_frames < 120) return;
  lx_frames = 0;
  if (lx_skipped) NETLOG("versus predict: %d machine-steps left to Compensate's own estimate (age under -0.25 s or over 1 s, or negative with MT3DX_NEGAGE=0) in the last 2 s\n", lx_skipped);
  lx_skipped = 0;
  for (int k = 0; k < CAR_LINK_MACHINES; ++k) {
    if (!lx_err[k].n) continue;
    double n = (double)lx_err[k].n;
    NETLOG("versus predict: pcb %d: position error against where it really was (%d samples, m): the game's compensation avg %.2f max %.2f, uncompensated %.2f, plain formula %.2f\n", k, lx_err[k].n,
            lx_err[k].comp / n, lx_err[k].comp_max, lx_err[k].raw / n, lx_err[k].lin / n);
    memset(&lx_err[k], 0, sizeof lx_err[k]);
  }
  for (int b = 0; b < 4; ++b) {
    if (!lx_bk[b].n) continue;
    static const char *const NAME[4] = { "0-5", "6-15", "16-30", "31-120" };
    NETLOG("versus predict: after a contact, frames %s: error avg %.2f max %.2f m (%d samples)\n", NAME[b], lx_bk[b].sum / (double)lx_bk[b].n, lx_bk[b].max, lx_bk[b].n);
    memset(&lx_bk[b], 0, sizeof lx_bk[b]);
  }
}

/* a contact the app has seen (its own change of velocity with a rival close): from here the scoring files the errors by the time since */
void car_link_contact(void) { lx_contact_cnt = M32(LX_NCNT); }

/* what a machine's own car does from one packet to the next: a jump past what its velocity gives (the car moved by the game, not by driving: a hit's
 * push out of the other car) and a turn of more than a few degrees in one frame; the first of them logged. */
static float sp_pos[CAR_LINK_MACHINES][3], sp_vel[CAR_LINK_MACHINES][3];
static uint32_t sp_cnt[CAR_LINK_MACHINES];
static int sp_ok[CAR_LINK_MACHINES], sp_logged, sp_trace_left, sp_traces, sp_cal;
static void sp_reset(void) {
  memset(sp_ok, 0, sizeof sp_ok);
  sp_logged = 0;
  sp_trace_left = sp_traces = sp_cal = 0;
  memset(an_ok, 0, sizeof an_ok);
  memset(an_rate, 0, sizeof an_rate);
}
#define AN_DEG (360.0f / 65536.0f)
static void sp_look(int pcb, const unsigned char *packet, uint32_t count) {
  float pos[3], vel[3];
  int16_t e16[3];
  memcpy(pos, packet + LX_POS, sizeof pos);
  memcpy(vel, packet + LX_VEL, sizeof vel);
  memcpy(e16, packet + 0x210, sizeof e16); /* (CEuler: three 16-bit angles, 65536 to a turn, and the order after them) */
  double turn = 0;
  if (an_ok[pcb] && count > an_prev_cnt[pcb] && count - an_prev_cnt[pcb] <= 6) { /* the angles' rate, per frame, kept smoothed: the prediction of its turning */
    float dc = (float)(count - an_prev_cnt[pcb]);
    for (int j = 0; j < 3; ++j) {
      float d = (float)(int16_t)(e16[j] - an_prev[pcb][j]), r = d / dc;
      if (fabsf(d / dc) * AN_DEG > turn) turn = (double)fabsf(r) * AN_DEG;
      r = r > AN_RATE_MAX ? AN_RATE_MAX : r < -AN_RATE_MAX ? -AN_RATE_MAX : r;
      an_rate[pcb][j] = 0.5f * an_rate[pcb][j] + 0.5f * r;
    }
  }
  memcpy(an_prev[pcb], e16, sizeof e16), an_prev_cnt[pcb] = count, an_ok[pcb] = 1;
  if (sp_ok[pcb] && count == sp_cnt[pcb] + 1) {
    double jump = 0;
    for (int j = 0; j < 3; ++j) {
      double d = (double)pos[j] - (double)sp_pos[pcb][j] - (double)sp_vel[pcb][j] * (1.0 / 60.0);
      if (!isfinite(d)) d = 0.0;
      jump += d * d;
    }
    double jm = sqrt(jump) * 0.001;
    if (turn > 5.0 && count > 30 && sp_trace_left <= 0 && sp_traces < 3) sp_trace_left = 26, sp_traces++;
    if ((jm > 0.3 || turn > 6.0) && count > 30 && sp_logged < 60) {
      sp_logged++;
      int since = (int)(int32_t)(count - lx_contact_cnt);
      NETLOG("versus spike: pcb %d count %u: its own car jumped %.2f m past its velocity, turned %.1f deg in one frame (angles %.0f %.0f %.0f deg), %d frames after my last contact\n", pcb, count, jm, turn,
              (double)e16[0] * AN_DEG, (double)e16[1] * AN_DEG, (double)e16[2] * AN_DEG, lx_contact_cnt == 0xffffffffu ? -1 : since);
    }
  }
  if ((count % 90) == 0 && count > 120 && sp_cal < 16) { /* the angles against the velocity's heading: which of the three is the heading, and which way */
    float sp = sqrtf(vel[0] * vel[0] + vel[2] * vel[2]);
    if (sp > 15000.0f) {
      sp_cal++;
      NETLOG("versus calib: pcb %d count %u angles %.1f %.1f %.1f deg, vel %.0f %.0f %.0f, heading atan2(vx,vz) %.1f atan2(-vx,vz) %.1f deg\n", pcb, count, (double)e16[0] * AN_DEG, (double)e16[1] * AN_DEG, (double)e16[2] * AN_DEG,
              (double)vel[0], (double)vel[1], (double)vel[2], (double)atan2f(vel[0], vel[2]) * 57.29578, (double)atan2f(-vel[0], vel[2]) * 57.29578);
    }
  }
  if (sp_trace_left > 0) { /* the packets around a turn, whole: what the car was doing */
    sp_trace_left--;
    float acc[3], spd;
    memcpy(acc, packet + LX_ACC, sizeof acc);
    memcpy(&spd, packet + 0x244, 4);
    NETLOG("versus trace: pcb %d count %u pos %.0f %.0f %.0f vel %.0f %.0f %.0f acc %.0f %.0f %.0f angles %.1f %.1f %.1f deg speed %.0f\n", pcb, count, (double)pos[0], (double)pos[1], (double)pos[2], (double)vel[0], (double)vel[1],
            (double)vel[2], (double)acc[0], (double)acc[1], (double)acc[2], (double)e16[0] * AN_DEG, (double)e16[1] * AN_DEG, (double)e16[2] * AN_DEG, (double)spd);
  }
  memcpy(sp_pos[pcb], pos, sizeof pos), memcpy(sp_vel[pcb], vel, sizeof vel);
  sp_cnt[pcb] = count, sp_ok[pcb] = 1;
}

/* a new packet from machine pcb: where that machine really was at its frame `count`, against what this machine had for it then */
static void lx_arrived(int pcb, const unsigned char *packet) {
  uint32_t count;
  float truth[3];
  memcpy(&count, packet + LX_COUNT, 4);
  if (count == lx_last[pcb]) return;
  lx_last[pcb] = count;
  lx_dbg_arr++;
  sp_look(pcb, packet, count);
  memcpy(truth, packet + LX_POS, sizeof truth);
  const lx_rec *r = &lx_ring[pcb][count & 255];
  if (!r->ok || r->cnt != count) return;
  lx_dbg_match++;
  double e[3] = { 0, 0, 0 };
  for (int j = 0; j < 3; ++j) {
    double a = r->comp[j] - truth[j], b = r->raw[j] - truth[j], c = r->lin[j] - truth[j];
    e[0] += a * a, e[1] += b * b, e[2] += c * c;
  }
  double ec = sqrt(e[0]) * 0.001; /* (the packet's positions are in millimetres) */
  lx_err[pcb].comp += ec, lx_err[pcb].raw += sqrt(e[1]) * 0.001, lx_err[pcb].lin += sqrt(e[2]) * 0.001;
  if (ec > lx_err[pcb].comp_max) lx_err[pcb].comp_max = ec;
  {
    int since = (int)(int32_t)(count - lx_contact_cnt);
    if (lx_contact_cnt != 0xffffffffu && since >= 0 && since < 120) {
      int b = since < 6 ? 0 : since < 16 ? 1 : since < 31 ? 2 : 3;
      lx_bk[b].sum += ec, lx_bk[b].n++;
      if (ec > lx_bk[b].max) lx_bk[b].max = ec;
    }
  }
  lx_err[pcb].n++;
}

void car_link_deliver(int pcb, const unsigned char *packet, int len) {
  if (!rc_link_rx || pcb < 0 || pcb >= CAR_LINK_MACHINES || len < CAR_LINK_PACKET || (link_gone >> pcb & 1u)) return;
  lx_arrived(pcb, packet);
  memcpy(VA(rc_link_rx + LINK_BUF * (uint32_t)pcb), packet, CAR_LINK_PACKET);
}

/* clCarDispManager::setupOtherCar: a traffic car comes on the road; its model */
static int on_other_setup(uint32_t slot, uint32_t type) {
  int i = (int)slot - TRAFFIC_SLOT0;
  if (i >= 0 && i < CAR_TRAFFIC) P.traffic_model[i] = type - TRAFFIC_TYPE0 < 4 ? TRAFFIC_MODEL[type - TRAFFIC_TYPE0] : 0;
  return 1;
}

static void first_frame(car_state *c);
static int phys_have; /* read_phys has a last frame's velocity to work from */

/* The gear the player was in when a race that is running went back to a countdown (a cutscene of a story or 10-in-a-row race that ends in a rolling start again: the arcade's
 * transmission goes back to SetFirst's gear, 2nd, and the driver's gear was lost: 4th became 2nd after the scene, issue 41). Kept from the last running step and put back in
 * the engine (and the manual's lever) for as long as the countdown runs. */
static struct { int running, keep, active, orig, forced; float spd; } GK; /* (spd: the speed of the last running step; orig: the engine's own gear when the keeping began, forced: it has been written over) */
/* A story select's second race starts as a new race after its movie (car_start twice), but the driver is still the one who was in the first race's gear (issue 41,
 * chapter 15: 4th before the scene, 2nd after): the gear the first race ended in, held through the second one's countdown like GK's. 0 none. */
static int CARRY, CARRY_LIVE; /* (live: a race has started since it was kept: the first one's last steps do not use it up) */
void car_carry_gear(int on) { CARRY = on && GK.keep >= 1 ? GK.keep : 0; }

/* The black box (always on, a few lines at most): a race step where the car turns more than 3.4 degrees in one step (200 degrees a second) or the eye moves by more than 0.3 m
 * against the car writes the 45 steps before it and the 45 after to the log, as "bb" lines (the car's place, heading, speed, the steering as read and as given, the eye,
 * the nearest traffic car, the wall, the slip), to see what was behind a sudden swing of the view (the car, the camera or the traffic). At most 6 times a race. */
typedef struct { float t, pos[3], yaw, spd, st_in, st, eye[3], raw[3], fx[4], dtraf, wall, slip, ang; int ps; } bb_rec;
static float RAWCAM[3]; /* the arcade's camera place of this frame before the shake filter (the world's) */
static struct { int n, post, quiet, dumps, head, count, view; float rel[3], yaw, ang; bb_rec r[45]; } BB;

static void bb_line(const char *tag, int k) {
  const bb_rec *e = &BB.r[k];
  fprintf(stderr, "bb%s t=%.2f pos=%.1f,%.1f,%.1f yaw=%.3f spd=%.1f in=%.2f st=%.2f eye=%.1f,%.1f,%.1f raw=%.1f,%.1f,%.1f fx=%.2f,%.2f,%.2f,%.2f traf=%.1f wall=%.2f slip=%.3f ang=%.2f ps=%d\n", tag, (double)e->t, (double)e->pos[0], (double)e->pos[1],
          (double)e->pos[2], (double)e->yaw, (double)e->spd, (double)e->st_in, (double)e->st, (double)e->eye[0], (double)e->eye[1], (double)e->eye[2], (double)e->raw[0], (double)e->raw[1], (double)e->raw[2], (double)e->fx[0], (double)e->fx[1], (double)e->fx[2], (double)e->fx[3], (double)e->dtraf, (double)e->wall,
          (double)e->slip, (double)e->ang, e->ps);
}

static void bb_step(const car_state *c) {
  if (c->race.play_state != 2 || c->race.countdown > 0.0f) {
    BB.n = 0;
    return;
  }
  float dtraf = 1e9f;
  for (int i = 0; i < c->n_traffic && i < CAR_TRAFFIC; ++i) {
    float dx = c->traffic[i].body[12] - c->pos[0], dy = c->traffic[i].body[13] - c->pos[1], dz = c->traffic[i].body[14] - c->pos[2];
    float d = sqrtf(dx * dx + dy * dy + dz * dz);
    if (d < dtraf) dtraf = d;
  }
  float rel[3], dyaw = c->yaw - BB.yaw;
  for (int k = 0; k < 3; ++k) rel[k] = c->cam_pos[k] - c->pos[k];
  while (dyaw > 3.14159265f) dyaw -= 6.2831853f;
  while (dyaw < -3.14159265f) dyaw += 6.2831853f;
  const int warm = BB.n++ > 30 && c->view == BB.view; /* (a change of view is a cut, not a jump) */
  BB.view = c->view;
  float jump = sqrtf((rel[0] - BB.rel[0]) * (rel[0] - BB.rel[0]) + (rel[1] - BB.rel[1]) * (rel[1] - BB.rel[1]) + (rel[2] - BB.rel[2]) * (rel[2] - BB.rel[2]));
  /* the view's direction against the car's body (degrees): a steady one is a rigid view */
  float ang;
  {
    const float *V = c->cam_view, *B = c->body;
    float dot = -(V[2] * B[8] + V[6] * B[9] + V[10] * B[10]);
    ang = acosf(fminf(1.0f, fmaxf(-1.0f, dot))) * 57.29578f;
  }
  const float dang = ang - BB.ang;
  BB.ang = ang;
  BB.yaw = c->yaw;
  memcpy(BB.rel, rel, sizeof rel);
  bb_rec *e = &BB.r[BB.head];
  e->t = c->race.total;
  memcpy(e->pos, c->pos, sizeof e->pos);
  e->yaw = c->yaw, e->spd = c->speed * 3.6f, e->st_in = c->steer_in, e->st = c->steer;
  memcpy(e->eye, c->cam_pos, sizeof e->eye);
  memcpy(e->raw, RAWCAM, sizeof e->raw);
  memcpy(e->fx, c->camfx, sizeof e->fx);
  e->ang = ang;
  e->dtraf = dtraf > 1e8f ? -1.0f : dtraf, e->wall = c->wall_hit, e->slip = c->phys.slip_body, e->ps = c->race.play_state;
  BB.head = (BB.head + 1) % 45;
  if (BB.count < 45) ++BB.count;
  if (BB.post > 0) {
    bb_line("+", (BB.head + 44) % 45);
    if (--BB.post == 0) BB.quiet = 120;
    return;
  }
  if (BB.quiet > 0) --BB.quiet;
  if (!warm || BB.quiet > 0 || BB.dumps >= 6) return;
  if (fabsf(dyaw) > 0.06f || jump > 3.0f || (fabsf(dang) > 0.6f && !BB.post) || (getenv("MT3DX_BBTEST") && BB.n == atoi(getenv("MT3DX_BBTEST")))) { /* (MT3DX_BBTEST=<n>: a trigger at the race's n-th step, to see the lines) */
    ++BB.dumps;
    fprintf(stderr, "bb: trigger %d: the car turned %.1f degrees in a step, the eye moved %.2f against the car (%s view)\n", BB.dumps, (double)(dyaw * 57.29578f), (double)jump,
            c->view == CAR_VIEW_ROOF ? "roof" : "other");
    for (int k = 0; k < BB.count; ++k) bb_line("-", (BB.head + 45 - BB.count + k) % 45);
    BB.post = 45;
  }
}

int car_start(car_state *c, int course, int manual, const car_spec *spec) {
  /* a fresh arcade memory image per race, like a fresh boot of CSeq::Init */
  P.ready = 0;
  phys_have = 0;
  memset(&GK, 0, sizeof GK);
  memset(&BB, 0, sizeof BB);
  memset(&RH, 0, sizeof RH);
  CARRY_LIVE = CARRY != 0;
  if (CARRY) GK.keep = CARRY, GK.active = 1;
  if (course < 0|| course >= CAR_COURSES || rc_load_image(P.image) != 0) return -1;
  P.course = course;
  P.manual = manual != 0;
  P.spec = spec ? *spec : CAR_SPEC_FD3S;
  P.top_lever = P.spec.gears - 1;
  memset(P.traffic_model, 0, sizeof P.traffic_model);
  rc_reset_heap();
  /* the 10-in-a-row's rivals' car types: the arcade's table by character (GetTenFightCarType, 0x9778fc0), as the port knows them (tendb.py: the
   * export build's Shima still says the Gemballa 964, whose car data it lacks: the rival was never made and the race ended at the start) */
  for (int i = 0; i < P.n_ten; ++i)
    if (P.ten[i].character >= 0 && P.ten[i].character < 32) W32(RCA(9778fc0) + 4u * (uint32_t)P.ten[i].character, P.ten[i].type);
  rc_on_hook = on_hook;
  rc_on_other_setup = on_other_setup;
  /* a time attack with the traffic on, as with the arcade's hidden button held at the course select */
  uint32_t cfg = rc_alloc(0x200);
  W32(cfg + CFG_CAR, P.spec.type);
  W32(cfg + CFG_POWER, (uint32_t)P.spec.power);
  W32(cfg + CFG_HANDLING, (uint32_t)P.spec.handling);
  W32(cfg + CFG_TRANS, P.manual ? TRANS_MT : TRANS_AT);
  W32(cfg + CFG_STAGE, COURSES[course].stage);
  W32(cfg + CFG_AREA, COURSES[course].area);
  WF(cfg + CFG_DIST, COURSES[course].dist);
  WF(cfg + CFG_GOAL, COURSES[course].dist + MF(TA_LENGTHS + 8 * (COURSES[course].stage - TA_STAGE0) + 4));
#ifdef WMMT_EGL
  if (getenv("WMMT_TA_GOAL")) /* test: the goal this many m from the start */
    WF(cfg + CFG_GOAL, COURSES[course].dist + 1000.0f * (float)atof(getenv("WMMT_TA_GOAL")));
#endif
  W32(cfg + CFG_MODE, MODE_TIME_ATTACK);
  /* the time attack's traffic: off, as the arcade's (clThread_6_2_CourseSelect::run 0x8739b29:
   * setTaOtherCar(1) only with a hidden button, input bit 0x40, held as the course is decided), on with the course select's TRAFFIC key (car_set_ta_traffic) */
  W32(cfg + CFG_TA_TRAFFIC, P.ta_traffic ? 1u : 0u);
  if (P.has_rival) { /* the story mode's race against its rivals */
    W32(cfg + CFG_MODE, MODE_STORY);
    W32(cfg + CFG_LEVEL, (uint32_t)P.rival[0].level);
    W32(cfg + CFG_LOSSES, (uint32_t)P.rival[0].losses);
    /* the save is the card: with one, a car tuned 1 step or less meets the exhibition
     * characters' easy settings from series 36 on (CSeq::Init) */
    W8(cfg + CFG_CARD, 1u);
    W32(cfg + CFG_RIVALS, (uint32_t)P.n_rival);
    for (int i = 0; i < P.n_rival; ++i) {
      W32(cfg + CFG_RIVAL_CHAR + 4 * (uint32_t)i, (uint32_t)P.rival[i].character);
      W32(cfg + CFG_RIVAL_CAR + 4 * (uint32_t)i, P.rival[i].type);
    }
    W32(cfg + CFG_SERIES, (uint32_t)P.rival[0].series);
    /* the series' own start and goal, as the story's menus set them (GetConfigRaceField_Story:
     * the SConfigRaceField is the first 6 words of the series' config) */
    uint32_t field = rc_alloc(0x40);
    call(f_899b378, 4, field, STORY_CONFIG, 0u, (uint32_t)P.rival[0].series);
    for (int i = 0; i < 6; ++i) W32(cfg + RACE_FIELD + 4 * i, M32(field + 4 * i));
    if (getenv("MT3DX_STORY_KM")) WF(cfg + CFG_GOAL, MF(cfg + CFG_DIST) + 1000000.0f * (float)atof(getenv("MT3DX_STORY_KM"))); /* test: a story race of this many km */
    fprintf(stderr, "story series %d (loop %d, lost %d): stage %u area %u dist %.0f goal %.0f\n", P.rival[0].series,
            P.rival[0].level, P.rival[0].losses, M32(cfg + CFG_STAGE),
            M32(cfg + CFG_AREA), MF(cfg + CFG_DIST), MF(cfg + CFG_GOAL));
  }
  rc_link_tx = rc_link_rx = 0;
  rc_on_link_receive = rc_on_link_send = NULL;
  if (P.vs_on) { /* a versus (clRaceThread::initJoin) */
    W32(cfg + CFG_MODE, MODE_BATTLE);
    W32(cfg + CFG_TA_TRAFFIC, 0u);
    uint32_t field = rc_alloc(0x40), eramp = M32(M32(JOIN_RAMPS + 4u * (uint32_t)P.vs_ramp) + 8);
    call(f_8992604, 2, field, eramp);
    for (int i = 0; i < 6; ++i) W32(cfg + RACE_FIELD + 4 * i, M32(field + 4 * i));
    WF(cfg + CFG_GOAL, MF(cfg + CFG_DIST) + P.vs_km * 1000000.0f);
    W8(cfg + CFG_LINK_NO_BAND, 0u);
    /* the places: PCB ids in order (initJoin's from the join list) */
    int place = 0;
    for (int i = 0; i < P.vs_n; ++i) place += P.vs[i].pcb < P.vs_pcb;
    W32(cfg + CFG_LINK_PLACE, (uint32_t)place);
    W32(cfg + CFG_LINK_OTHERS, (uint32_t)P.vs_n);
    for (int i = 0; i < P.vs_n; ++i) {
      int pl = P.vs[i].pcb > P.vs_pcb;
      for (int j = 0; j < P.vs_n; ++j) pl += P.vs[j].pcb < P.vs[i].pcb;
      W32(cfg + CFG_LINK_PCB + 4u * (uint32_t)i, (uint32_t)P.vs[i].pcb);
      W32(cfg + CFG_LINK_OTHER_PLACE + 4u * (uint32_t)i, (uint32_t)pl);
      W32(cfg + CFG_LINK_CAR + 4u * (uint32_t)i, P.vs[i].type);
      W32(cfg + CFG_LINK_POWER + 4u * (uint32_t)i, (uint32_t)P.vs[i].power);
      W32(cfg + CFG_LINK_HANDLING + 4u * (uint32_t)i, (uint32_t)P.vs[i].handling);
    }
    W32(cfg + CFG_LINK_RANDOM, P.vs_seed);
    rc_link_pcb = (uint32_t)P.vs_pcb;
    rc_link_tx = rc_alloc(LINK_BUF);
    rc_link_rx = rc_alloc(LINK_BUF * CAR_LINK_MACHINES);
    rc_on_link_receive = link_receive;
    rc_on_link_send = link_send;
    fprintf(stderr, "versus pcb %d place %d others %d ramp %d (ERamp %u): stage %u area %u dist %.0f goal %.0f\n", P.vs_pcb, place,
            P.vs_n, P.vs_ramp, eramp, M32(cfg + CFG_STAGE), M32(cfg + CFG_AREA), MF(cfg + CFG_DIST), MF(cfg + CFG_GOAL));
  }
  if (P.ten_stage) { /* the 10-in-a-row (clRaceThread::initTen): its level and course's start */
    W32(cfg + CFG_MODE, MODE_TEN_FIGHT);
    W32(cfg + CFG_TA_TRAFFIC, 0u);
    W32(cfg + CFG_TEN_LEVEL, (uint32_t)P.ten_level);
    uint32_t field = rc_alloc(0x40);
    call(f_89923e4, 3, field, (uint32_t)P.ten_stage, (uint32_t)P.ten_level);
    for (int i = 0; i < 6; ++i) W32(cfg + RACE_FIELD + 4 * i, M32(field + 4 * i));
    fprintf(stderr, "ten stage %d level %d: area %u dist %.0f goal %.0f\n", P.ten_stage, P.ten_level, M32(cfg + CFG_AREA),
            MF(cfg + CFG_DIST), MF(cfg + CFG_GOAL));
  }
  call(f_89878c4, 1, cfg);
  P.vs_race = P.vs_on ? P.vs_n : 0;
  vs_time_km = P.vs_on ? P.vs_km : 0.0f;
  P.vs_on = 0; /* car_set_versus is for this race only */
  call(f_89a2584, 1, fbits(1.0f / 60.0f));
  call(f_89a2548, 1, fbits(1.0f));
  /* the time attack starts rolling (GetSeqStartOfs_Mc): CState's countdown (Prep_CountDown,
   * 4 s) has the pilot drive, then the race runs with the car in the player's hands */
  P.m = M32(MACHINE_PTR);
  P.gen = M32(OTHER_GEN_PTR);
  P.gin = rc_alloc(0x40);
  P.gin_ptr = rc_alloc(4);
  W32(P.gin_ptr, P.gin);
  P.disp = rc_alloc(0x260);
  P.eff = rc_alloc(0x100);
  P.cam = rc_alloc(0x100);
  P.mirror = rc_alloc(0x100);
  P.jct = rc_alloc(8);
  P.info_race = rc_alloc(0x40);
  P.info_map = rc_alloc(0x20);
  P.info_sign = rc_alloc(8);
  P.info_jct = rc_alloc(0x20);
  P.jct_line = rc_alloc(0x30);
  P.jct_prev = 0;
  memset(P.jct_saved, 0, sizeof P.jct_saved);
  P.lines = rc_alloc(48);
  P.ten_line = rc_alloc(24);
  P.title_log = rc_alloc(CAR_TITLE_LOG);
  P.info_story = rc_alloc(0x40);
  P.info_ten = rc_alloc(0xa0);
  P.result = rc_alloc(0x40);
  P.first_pt = rc_alloc(0x20);
  P.ready = 1;
  first_frame(c);
  return 0;
}

/* clCarDisp::update: tyre nodes +0x8c..+0x98 (FL, FR, RL, RR) take SInfoCarDisp +0x188, +0x148,
 * +0x208, +0x1c8; their spin nodes +0x9c..+0xa8 a turn about x by +0x248..+0x254 */
static void read_tyres(float tyre[4][16], float rot[4]) {
  static const uint32_t at[4] = { 0x188, 0x148, 0x208, 0x1c8 };
  for (int k = 0; k < 4; ++k) {
    memcpy(tyre[k], VA(P.disp + at[k]), sizeof tyre[k]);
    rot[k] = MF(P.disp + 0x248 + 4 * (uint32_t)k);
  }
}

static void read_effect(car_effect *e) {
  e->spark = MF(P.eff + 0x68);
  memcpy(e->at, VA(P.eff + 0x6c), sizeof e->at);
  memcpy(e->dir, VA(P.eff + 0x78), sizeof e->dir);
  for (uint32_t k = 0; k < 4; ++k) {
    e->track[k] = MF(P.eff + 0x40 + 4 * k);
    memcpy(e->wheel[k], VA(P.disp + 0x44 + 0x40 * k), sizeof e->wheel[k]);
  }
  e->backfire = M8(P.eff + 0x84) != 0;
  float vx = MF(P.eff + 0x30), vy = MF(P.eff + 0x34), vz = MF(P.eff + 0x38);
  e->speed = sqrtf(vx * vx + vy * vy + vz * vz);
  memcpy(e->smoke_at[0], VA(P.eff + 0x18), sizeof e->smoke_at[0]);
  memcpy(e->smoke_at[1], VA(P.eff + 0x24), sizeof e->smoke_at[1]);
  e->smoke_k[0] = MF(P.eff + 0x50), e->smoke_k[1] = MF(P.eff + 0x54);
  e->vel[0] = vx, e->vel[1] = vy, e->vel[2] = vz;
}

/* yt::CInfoSEMc (car.h car_se) */
static void read_se(car_se *s) {
  uint32_t se = P.m + 0xb88u;
  s->turbo = M8(se + 0x1c) != 0;
  s->turbin_vol = MF(se + 0x74), s->turbin_hz = (float)(int32_t)M32(se + 0x78);
  s->air_vol = MF(se + 0x7c), s->air_hz = (float)(int32_t)M32(se + 0x80);
  s->valb = M8(se + 0xa8) != 0, s->valb_vol = MF(se + 0xac);
  s->backfire = M8(se + 0xb0) != 0, s->backfire_vol = MF(se + 0xb4), s->backfire_hz = (float)(int32_t)M32(se + 0xb8);
  s->gyap = M8(se + 0xbc) != 0, s->gyap_vol = MF(se + 0xc0), s->gyap_hz = (float)(int32_t)M32(se + 0xc4);
}

static void read_body(car_state *c) {
  call(f_89b45d0, 2, P.m, P.disp);
  /* igMatrix44f is row-major with the translation in the last row: the same
   * memory layout as a GL column-major matrix. Units are already display units. */
  memcpy(c->body, VA(P.disp + 4), sizeof c->body);
  read_tyres(c->tyre, c->tyre_rot);
  c->brake = M8(P.disp + 0x144);
  c->spot = M8(P.disp + 0x146) == 1;
  call(f_89b410c, 2, P.m, P.eff);
  read_effect(&c->effect);
  const float *M = c->body;
  c->pos[0] = M[12];
  c->pos[1] = M[13];
  c->pos[2] = M[14];
  c->yaw = atan2f(M[8], M[10]);
  c->pitch = asinf(fmaxf(-1.0f, fminf(1.0f, M[9])));
  c->roll = atan2f(M[1], M[5]);
}

/* SInfoCarCamera: igMatrix44f view (world -> camera; row-major with the translation
 * last, so GL column-major as it stands), horizontal fov at +0x40, near plane at +0x44,
 * the radial blur at +0x48 (clRaceThread::yield hands it to clGraphicFrame::setBlur).
 * clRaceThread::race takes CSeq::GetInfoCarCamera's: CMachine's with the play view and true
 * (the camera sways with the car, CCamSway, so once per arcade frame), but in the
 * 10-in-a-row while a rival comes on (play state 1) CSeqCamTen's, which shows it arriving,
 * and once a story or 10-in-a-row race reaches its goal (state 8) CSeqCamGoal's. */
/* CAMERA SHAKE below full: the camera's place and axes in the car's own frame (turned by its yaw, so a corner is not a shake) are smoothed over about a fifth of a second and
 * blended with the arcade's own by the setting, which takes the shake of speed, rough road and kerbs out and keeps the camera's slower moves. Full leaves the view untouched. */
static int shake_tenths = 10;
static struct {
  int on, view;
  float off[3], ax[3][3]; /* the smoothed offset of the camera from the car, and its three axes (view matrix rows), in the car's frame */
} SK;

void car_shake(int tenths) { shake_tenths = tenths < 0 ? 0 : tenths > 10 ? 10 : tenths; }
static int shake_slow, slow_rej, slow_warm, far_n, init_n, init_wait; /* (init_wait: the steps the arcade's camera has gone on moving since the cut, see shake_filter) */
static float init_off[12][3], init_ax[12][3][3]; /* the samples a first-person lock's rest place is made of (the median of 12) */
#define FAR_CUT 180 /* steps a far camera stays far before it is taken for a cut (3 s) */ /* (the first-person lock: samples left out in a row, and steps since its rest place was set) */
void car_shake_slow(int on) { shake_slow = on; } /* (smooth motion's first-person view: the camera's offset and axes follow over about half a second, not a sixth) */

/* the frame the filter works in (rows: its three axes in the world): the car's turn about the up axis, so a corner is not a shake; or, for smooth motion's first-person view,
 * the car's body itself (its pitch and roll too), so the eye is held on the car and only slowly leaves it */
static float SF[3][3];
static void shake_to(const float in[3], float out[3]) {
  for (int k = 0; k < 3; ++k) out[k] = SF[k][0] * in[0] + SF[k][1] * in[1] + SF[k][2] * in[2];
}
static void shake_from(const float in[3], float out[3]) {
  for (int k = 0; k < 3; ++k) out[k] = SF[0][k] * in[0] + SF[1][k] * in[1] + SF[2][k] * in[2];
}

#define SHAKE_PASS 3.0f /* (units: about 40 cm) */
#define SHAKE_STOP 7.0f
static void shake_filter(car_state *c) {
  if ((shake_tenths >= 10 && !shake_slow) || M32(STATE) >= 4) { /* (and no smoothing once the race is over, play states 4 and up: the finish's cameras are the arcade's own moves around the car, and a filter that follows it a fifth of a second late only makes it wobble; issue 41) */
    SK.on = 0;
    return;
  }
  float V[16];
  memcpy(V, c->cam_view, sizeof V);
  if (shake_slow) {
    const float *B = c->body;
    for (int k = 0; k < 3; ++k) SF[k][0] = B[4 * k], SF[k][1] = B[4 * k + 1], SF[k][2] = B[4 * k + 2];
  } else {
    const float sy = sinf(c->yaw), cy = cosf(c->yaw);
    const float F[3][3] = { { cy, 0.0f, -sy }, { 0.0f, 1.0f, 0.0f }, { sy, 0.0f, cy } };
    memcpy(SF, F, sizeof SF);
  }
  float cam[3] = { c->cam_pos[0] - c->pos[0], c->cam_pos[1] - c->pos[1], c->cam_pos[2] - c->pos[2] }, off[3], ax[3][3];
  shake_to(cam, off);
  for (int r = 0; r < 3; ++r) {
    const float w[3] = { V[r], V[4 + r], V[8 + r] };
    shake_to(w, ax[r]);
  }
  float d = 0, len = 1e-3f;
  for (int k = 0; k < 3; ++k) d += (off[k] - SK.off[k]) * (off[k] - SK.off[k]), len += SK.off[k] * SK.off[k];
  float fwd = ax[2][0] * SK.ax[2][0] + ax[2][1] * SK.ax[2][1] + ax[2][2] * SK.ax[2][2];
  (void)len;
  /* (a cut, a new view, the first frame: no smoothing across it. A cut is metres: the road's jolts move the roof view's eye a metre or more in a few steps, and a limit
   * relative to the offset (about 3 units) made the filter give up exactly there) */
  /* (and only a sample that stays that far is a cut: at 310 km/h the roof camera's samples jump 2 to 4 m fore and aft for a step or two on a steady road, which the black box
   * showed; each was taken for a cut and the eye went there, and then slid back for seconds. A few steps of far samples are left out, and the eye stays where it was.) */
  const int far = d > 40.0f * 40.0f || fwd < 0.95f;
  { /* test: MT3DX_SKLOG=1 logs the first-person lock's state every 6th step: where the arcade's camera is against the car (off), the lock's rest place, the roll of the view against the rest's */
    static int lg = -2, n;
    if (lg == -2) lg = getenv("MT3DX_SKLOG") != NULL;
    if (lg && shake_slow && ++n % 6 == 0) {
      float roll = ax[0][0] * SK.ax[0][0] + ax[0][1] * SK.ax[0][1] + ax[0][2] * SK.ax[0][2];
      fprintf(stderr, "sk: n %d on %d init_n %d wait %d far_n %d rej %d warm %d off %.1f %.1f %.1f rest %.1f %.1f %.1f d %.1f fwd %.4f rollcos %.4f state %d cd %.2f\n", n, SK.on, init_n, init_wait, far_n, slow_rej, slow_warm, (double)off[0], (double)off[1],
              (double)off[2], (double)SK.off[0], (double)SK.off[1], (double)SK.off[2], (double)d, (double)fwd, (double)roll, (int)M32(STATE), (double)c->race.countdown);
    }
  }
  if (shake_slow && init_n > 0 && SK.on && SK.view == P.view) {
    /* the rest place is the median of the first 12 samples after a cut, not the one sample the cut met (which can be one of the camera's outliers: the black box had a lane change at
     * 323 km/h swing the roof camera 4 to 6 m back for 0.4 s, and every rule that took such a swing for a cut left the eye there for seconds); the view follows the camera till then */
    /* (and the camera has to have stopped moving first: a cut to the first-person view after a rival's introduction, or at a restart, has the arcade's camera gliding from where it was
     * to its place over a second or more, and a rest place made of the first 12 steps of that glide held the view tilted and off the road for seconds, the 10-in-a-row's next rival
     * on the Thor. Samples that are still moving, in a restart's or an introduction's countdown, start the 12 over, 10 s at most (where the log showed the glide from the introduction last 5 s and the lock taken in its middle, 80 units behind the car): the view follows the camera
     * itself meanwhile, as the arcade has it.) */
    if (init_n >= 1 && init_wait < 600 && (c->race.countdown > 0.0f || M32(STATE) != 2)) { /* (a restart's or an introduction's glide takes 5 s or so and ends slowly: a tenth of a unit a step is still a move. In a race at speed the camera's own noise is as large as its moves, so a cut there takes its 12 samples as they come) */
      const float *po = init_off[init_n - 1], *pz = init_ax[init_n - 1][2], *px = init_ax[init_n - 1][0];
      float dd = 0;
      for (int k = 0; k < 3; ++k) dd += (off[k] - po[k]) * (off[k] - po[k]);
      const float fz = ax[2][0] * pz[0] + ax[2][1] * pz[1] + ax[2][2] * pz[2], fx = ax[0][0] * px[0] + ax[0][1] * px[1] + ax[0][2] * px[2]; /* (the view's direction and its roll) */
      if (dd > 0.01f || fz < 0.99998f || fx < 0.99998f) {
        ++init_wait;
        init_n = 1;
        memcpy(init_off[0], off, sizeof off), memcpy(init_ax[0], ax, sizeof ax);
        return;
      }
    }
    memcpy(init_off[init_n], off, sizeof off), memcpy(init_ax[init_n], ax, sizeof ax);
    if (++init_n >= 12) {
      float t[12];
      for (int k = 0; k < 3; ++k) {
        for (int i = 0; i < 12; ++i) t[i] = init_off[i][k];
        for (int i = 1; i < 12; ++i) for (int j = i; j > 0 && t[j] < t[j - 1]; --j) { float u = t[j]; t[j] = t[j - 1]; t[j - 1] = u; }
        SK.off[k] = 0.5f * (t[5] + t[6]);
        for (int q = 0; q < 3; ++q) {
          for (int i = 0; i < 12; ++i) t[i] = init_ax[i][q][k];
          for (int i = 1; i < 12; ++i) for (int j = i; j > 0 && t[j] < t[j - 1]; --j) { float u = t[j]; t[j] = t[j - 1]; t[j - 1] = u; }
          SK.ax[q][k] = 0.5f * (t[5] + t[6]);
        }
      }
      init_n = 0;
      slow_warm = 0;
      init_wait = 0;
    }
    return;
  }
  if (!SK.on || SK.view != P.view || (far && ++far_n > FAR_CUT)) {
    SK.on = 1, SK.view = P.view, slow_rej = slow_warm = far_n = 0;
    memcpy(SK.off, off, sizeof off), memcpy(SK.ax, ax, sizeof ax);
    if (shake_slow) {
      init_n = 1, init_wait = 0;
      memcpy(init_off[0], off, sizeof off), memcpy(init_ax[0], ax, sizeof ax);
    }
    return;
  }
  if (!far) far_n = 0;
  float a = shake_slow ? 0.02f : 0.1f; /* (a step is 1/60 s: 0.1 of the way is a time constant of about 0.16 s) */
  float s = shake_tenths / 10.0f;
  if (shake_slow) {
    /* smooth motion's first-person view keeps the arcade's shake, as the 60 Hz game has it, by the CAMERA SHAKE option: the camera's moves about the lock's rest place are let through (the bumps, the
     * engine's and the road's tremble), but not the swings of a few metres the roof camera makes now and then at speed (the black box: 2 to 6 m fore and aft for a step or a few), which
     * are let through less the further they are and not at all past a limit */
    const float dist = sqrtf(d);
    s *= dist < SHAKE_PASS ? 1.0f : dist > SHAKE_STOP ? 0.0f : (SHAKE_STOP - dist) / (SHAKE_STOP - SHAKE_PASS);
  }
  if (shake_slow) {
    /* smooth motion's first-person view: the eye is held on the car's body. The arcade's roof camera is not rigid on it: at the same places of a course (the spawn, a bend) it swings
     * 2 m or more forward and back of the eye's rest place, over a few steps, which a slow filter only spreads out. So the rest place is learned (slowly, from the samples that are near it)
     * and the samples far from it are left out; if they stay far (the view really moved) the rest place goes to them after two seconds. */
    if (d < 25.0f && fwd > 0.998f) {
      slow_rej = 0;
      /* (the rest place is learned in the first 1.5 s after a cut, then it hardly moves: the arcade's camera samples carry a metre or two of noise of their own at speed, and learning
       * from them at 2 % a step let a part of it through as a slight wobble that could be felt; 0.4 % follows the slow changes (speed, the body's pitch) and no more) */
      a = slow_warm < 90 ? 0.06f : 0.004f;
      ++slow_warm;
    } else {
      /* (a transient is left out, but the rest place does not wait and snap: the log's black box showed the roof camera sitting 0.6 to 0.7 m to one side of it for good, and the old
       * "stay far for two seconds, then go to the sample" made the view jump that far in one step, 5 times in a 100 s run. It slides towards the sample at 0.15 units a step (9 a second):
       * a swing of a few steps moves it a hair, a lasting move is followed in under a second) */
      a = 0.0f;
      ++slow_rej;
      if (!far && slow_rej > 30) { /* (a swing of a few tenths of a second is not followed at all) */
        const float dist = sqrtf(d), st = dist > 0.15f ? 0.15f / dist : 1.0f;
        for (int k = 0; k < 3; ++k) SK.off[k] += (off[k] - SK.off[k]) * st;
        for (int r = 0; r < 3; ++r)
          for (int k = 0; k < 3; ++k) SK.ax[r][k] += (ax[r][k] - SK.ax[r][k]) * 0.01f;
      }
    }
  }
  float o[3], x[3][3];
  for (int k = 0; k < 3; ++k) {
    SK.off[k] += (off[k] - SK.off[k]) * a;
    o[k] = SK.off[k] + (off[k] - SK.off[k]) * s;
    for (int r = 0; r < 3; ++r) SK.ax[r][k] += (ax[r][k] - SK.ax[r][k]) * a, x[r][k] = SK.ax[r][k] + (ax[r][k] - SK.ax[r][k]) * s;
  }
  /* an orthonormal frame again: the view direction (axis 2) kept, the others made square to it */
  float n2 = sqrtf(x[2][0] * x[2][0] + x[2][1] * x[2][1] + x[2][2] * x[2][2]);
  if (n2 < 1e-4f) return;
  for (int k = 0; k < 3; ++k) x[2][k] /= n2;
  float dp = x[0][0] * x[2][0] + x[0][1] * x[2][1] + x[0][2] * x[2][2];
  for (int k = 0; k < 3; ++k) x[0][k] -= dp * x[2][k];
  float n0 = sqrtf(x[0][0] * x[0][0] + x[0][1] * x[0][1] + x[0][2] * x[0][2]);
  if (n0 < 1e-4f) return;
  for (int k = 0; k < 3; ++k) x[0][k] /= n0;
  x[1][0] = x[2][1] * x[0][2] - x[2][2] * x[0][1];
  x[1][1] = x[2][2] * x[0][0] - x[2][0] * x[0][2];
  x[1][2] = x[2][0] * x[0][1] - x[2][1] * x[0][0];
  /* back to the world: the inverse turn */
  float p[3], R[3][3];
  shake_from(o, p);
  for (int r = 0; r < 3; ++r) shake_from(x[r], R[r]);
  for (int r = 0; r < 3; ++r) V[r] = R[r][0], V[4 + r] = R[r][1], V[8 + r] = R[r][2];
  for (int k = 0; k < 3; ++k) c->cam_pos[k] = c->pos[k] + p[k];
  for (int r = 0; r < 3; ++r) V[12 + r] = -(R[r][0] * c->cam_pos[0] + R[r][1] * c->cam_pos[1] + R[r][2] * c->cam_pos[2]);
  memcpy(c->cam_view, V, sizeof V);
}

static void read_camera(car_state *c) {
  W32(PLAY_VIEW, (uint32_t)P.view);
  call(f_8982338, 1, P.cam);
  memcpy(c->cam_view, VA(P.cam), sizeof c->cam_view);
  const float *V = c->cam_view;
  for (int j = 0; j < 3; j++) c->cam_pos[j] = -(V[12] * V[4 * j] + V[13] * V[4 * j + 1] + V[14] * V[4 * j + 2]);
  memcpy(RAWCAM, c->cam_pos, sizeof RAWCAM);
  shake_filter(c);
  c->cam_fov = MF(P.cam + 0x40);
  c->cam_near = MF(P.cam + 0x44);
  c->cam_blur = fminf(1.0f, fmaxf(0.0f, MF(P.cam + 0x48)));
  /* the back mirror's (clRaceThread::yield: CSeq::GetInfoMirrorCamera, CMachine's
   * GetInfoCarMirror: no sway), only for the view from the roof, the one that shows it */
  c->view = P.view;
  c->mirror = P.view == CAR_VIEW_ROOF && P.mirror && (call(f_8982580, 1, P.mirror) & 0xff);
  if (c->mirror) {
    memcpy(c->mirror_view, VA(P.mirror), sizeof c->mirror_view);
    c->mirror_fov = MF(P.mirror + 0x40);
    c->mirror_near = MF(P.mirror + 0x44);
  }
}

void car_view(int view) { P.view = view; }

/* SInfoJctBlockDisp, as CSeq::GetInfoJctBlockDisp gets it for clCourseArrow: the two
 * junction blocks (arrow boards closing the branches off the course) to show now. */
static void read_jct_blocks(car_state *c) {
  call(f_89c4000, 3, P.jct, M32(P.m + M_AREA_PROC), P.m + M_AREA_PROC + 4);
  for (int i = 0; i < 2; i++) c->jct_block[i] = (int)M32(P.jct + 4 * i);
}

/* The time attack's consoles: S2DInfoRace (+4 time left, +8 time since the start, +0xc mm
 * to the goal) and S2DInfoMap (+0 area, +4 the next, +8 z, +0xc x in display units, +0x10
 * heading), as CSeq hands them to clStandardConsole and clMiniMap; the sections from the
 * CTimeKeeper the way clWorkSpace gets them for the section times. */
static void read_race(car_race *r) {
  call(f_8983950, 1, P.info_race);
  call(f_8983874, 1, P.info_map);
  call(f_898380c, 1, P.info_sign);
  memset(VA(P.info_jct), 0, 0x20);
  call(f_8982f4c, 1, P.info_jct);
  r->jct_state = (int)M32(P.info_jct), r->jct_area[0] = (int)M32(P.info_jct + 4), r->jct_area[1] = (int)M32(P.info_jct + 8);
  /* the wall is read as it goes to deciding (clCourseJunctionManager::update sets its two boards up then) */
  if (r->jct_state == 1 && P.jct_prev != 1) {
    call(f_8983cd8, 3, P.jct_line, P.jct_line + 12, P.jct_line + 24);
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) P.jct_saved[i][j] = MF(P.jct_line + 12u * (uint32_t)i + 4u * (uint32_t)j);
  }
  P.jct_prev = r->jct_state;
  memcpy(r->jct_line, P.jct_saved, sizeof r->jct_line);
  r->countdown = MF(P.info_race);
  r->time_left = MF(P.info_race + 4);
  r->remain = fmaxf(0.0f, MF(P.info_race + 0xc) * 0.001f);
  uint32_t st = M32(STATE);
  int was = r->state;
  r->play_state = (int)st;
  r->state = st >= 7 && st <= 8 ? CAR_RACE_GOAL : st >= 4 && st <= 6 ? CAR_RACE_TIME_UP : CAR_RACE_RUN;
  /* clRaceThread::race returns on clWorkSpace::getPlayState 8 (setGoal) or 6 (setTimeUp);
   * in the time attack GoalWait (7) gives way to Goal the next frame (Ck_GoalWait_2_Goal).
   * A story race also ends on 3 (Defeat) and 10 (Suspend: Ck_X_2_Suspend, the odometer past
   * the episode's distance at 0x9802a7c; clRaceThread::race fades out and returns). */
  r->over = st == 8 || st == 6 || st == 3 || st == 10;
  /* the arcade's total runs on after the time is up; the console's stops there */
  if (r->state != CAR_RACE_TIME_UP || was != CAR_RACE_TIME_UP) r->total = MF(P.info_race + 8);
  uint32_t tk = M32(TIME_KEEPER_PTR);
  int n = tk ? (int)M32(tk + TK_COUNT) : 0, next = tk ? (int)M32(tk + TK_NEXT) : 0;
  int section = next - 1;
  if (n > 1 && section > r->section && section < n - 1) /* a checkpoint passed (not the goal) */
    r->extended = MF(M32(tk + TK_EXTEND) + 4 * (uint32_t)(next - 1));
  r->sections = n - 1 < CAR_SECTIONS ? n - 1 : CAR_SECTIONS;
  r->section = section < 0 ? 0 : section;
  for (int k = 0; k < next && k <= CAR_SECTIONS; k++) r->split[k] = MF(M32(tk + TK_SPLIT) + 4 * (uint32_t)k);
  r->area = (int)M32(P.info_map);
  { /* CSeq::GetInfoMyCarPlace */
    uint32_t m = M32(MACHINE_PTR);
    r->place = m ? (int)M32(M32(m + 0x93cu)) : 0;
    r->place_at = m ? MF(m + 0x7b4u + 0x1bcu) - MF(m + 0x7b4u + 0x180u) : 0.0f;
  }
  r->next_area = (int)M32(P.info_map + 4);
  r->map_z = MF(P.info_map + 8);
  r->map_x = MF(P.info_map + 0xc);
  r->map_angle = MF(P.info_map + 0x10);
  r->sign = (int)M32(P.info_sign + 4);
  { /* GetMapText(EAreaId, float) 0x899f4d0 (a stub in the recompile): table 0x9778980 of
     * {area, text below, threshold, text at or above}, keyed by the CSlit's 0x1bc - 0x180 */
    uint32_t slit = M32(RCA(98028e4)) + 0x7b4u, row = RCA(9778980) + 16u * (uint32_t)r->area;
    float at = MF(slit + 0x1bc) - MF(slit + 0x180);
    r->map_text = r->area >= 0 && r->area < 75 ? (int)M32(MF(row + 8) > at ? row + 4 : row + 0xc) : 0;
  }
  /* clGoalLine::clLine::update: the goal's row while the race info has the goal ahead, the
   * checkpoint's whenever CSeq gives one */
  r->lines = 0;
  if (MF(P.info_race + 0xc) > 0.0f && (call(f_898f518, 2, P.lines, P.lines + 12) & 0xffu)) r->lines |= 1;
  if (call(f_898f6d4, 2, P.lines + 24, P.lines + 36) & 0xffu) r->lines |= 2;
  memcpy(r->line, VA(P.lines), sizeof r->line);
  if (P.ten_stage) {
    /* S2DInfoTenFight: +0 rivals beaten, +4.. the s each took, +0x84 s on the one now, +0x88
     * the rolling start's gear, +0x8c mm behind it (float), +0x90/+0x91 SwName, SwGear */
    memset(VA(P.info_ten), 0, 0xa0);
    call(f_8982be8, 1, P.info_ten);
    r->ten_beaten = (int)M32(P.info_ten);
    r->ten_rival = (int)M32(TEN_RIVAL_NO);
    for (int k = 0; k < 32; ++k) r->ten_time[k] = MF(P.info_ten + 4 + 4 * (uint32_t)k);
    r->ten_now = MF(P.info_ten + 0x84);
    r->ten_gear = (int)M32(P.info_ten + 0x88);
    r->ten_gap = MF(P.info_ten + 0x8c) * 0.001f;
    r->ten_sw_name = M8(P.info_ten + 0x90);
    r->ten_sw_gear = M8(P.info_ten + 0x91);
    uint32_t ci = M8(CUTIN_ON) ? M32(CUTIN_INFO) : 0;
    r->cut_char = ci ? (int)M32(ci + 0x4c) : -1;
    r->cut_start = ci && M8(ci + 0x50);
    r->cut_lose = ci && M8(ci + 0x51);
    r->ten_line_on = (call(f_8983e5c, 2, P.ten_line, P.ten_line + 12) & 0xffu) && st == 2;
    memcpy(r->ten_line, VA(P.ten_line), sizeof r->ten_line);
    /* all ten beaten (Goal): SPlayResult +0x14, CState::GetTenFightResult -> CTenSeq::GetResult
     * of the time left: 3 gold, 2 silver, 1 bronze (the workspace's TenResult, the card's) */
    if (st == 8 && !r->ten_medal) {
      call(f_898317c, 1, P.result);
      r->ten_medal = (int)M32(P.result + 0x14);
    }
  }
  if (P.vs_race) {
    /* yt::CSeq::Get2DInfoBattle (clJoinConsole's getInfoJoinRank / getInfoJoinAdvantage): the
     * player's odometer (CMachine +0x970) less each link rival's (CLxRival: +8 on the road,
     * +0x240 its packet, +0x30 there the odometer; 0 while it is not on), mm; the rank counts
     * those ahead */
    uint32_t m = M32(MACHINE_PTR);
    float odo = m ? MF(m + 0x970u) : 0.0f;
    r->vs_rank = 0;
    for (int i = 0; i < CAR_RIVALS; ++i) {
      uint32_t lx = M32(LX_RIVALS + 4u * (uint32_t)i);
      int gone = P.vs_race && i < P.vs_n && (link_gone >> P.vs[i].pcb & 1u);
      if (gone && lx) W8(lx + 8u, 0); /* (a machine that left: off the road for the game too, out of the ranking) */
      r->vs_on[i] = lx && i < P.vs_race && !gone;
      if (!r->vs_on[i]) continue;
      { /* (measuring: a rival that goes off the road and on again changes the ranking and the last stretch's call with it) */
        static int was_on[CAR_RIVALS] = { -1, -1, -1 }, flips[CAR_RIVALS];
        int on = M8(lx + 8u) != 0;
        if (was_on[i] >= 0 && on != was_on[i] && flips[i]++ < 40) NETLOG("versus rival %d on the road: %d -> %d (its odometer %.0f m, mine %.0f m)\n", i, was_on[i], on, (double)(MF(M32(lx + 0x240u) + 0x30u) * 0.001f), (double)(odo * 0.001f));
        was_on[i] = on;
      }
      float d = odo - (M8(lx + 8u) ? MF(M32(lx + 0x240u) + 0x30u) : 0.0f);
      r->vs_gap[i] = d * 0.001f;
      if (d < 0.0f) r->vs_rank++;
    }
  }
  if (P.vs_race && (st == 3 || st == 6 || st == 8) && !r->vs_place) {
    /* a versus: SPlayResult +0 the place by the odometers (CState::Prep_Goal's ranking, 0 first;
     * the link's machines' from their finish packets), +0x10 whether the goal was reached */
    call(f_898317c, 1, P.result);
    r->vs_place = (int)M32(P.result) + 1;
    r->won = M32(P.result) == 0;
  }
  if ((st == 3 || st == 6 || st == 8 || st == 10) && !r->title_log_on) {
    /* the run's log for the titles (yt::CTitle's, CSeq::GetPlayLog) once it is over: the
     * _TitleRunningLog_ judgeTitle hands youTitle::GetTitleList at _TitleData_ +0x384 */
    memset(VA(P.title_log), 0, CAR_TITLE_LOG);
    call(f_898332c, 1, P.title_log);
    memcpy(r->title_log, VA(P.title_log), CAR_TITLE_LOG);
    r->title_log_on = 1;
  }
  if (P.has_rival) {
    /* S2DInfoStory +0: mm (a float) the rival is behind (negative: ahead). At the end (states 3
     * Defeat, 6 TimeUp, 8 Goal) SPlayResult has the player's place by the odometers
     * (CState::Prep_Goal's ranking, 0 first) at +0 and whether the goal was reached at +0x10. */
    call(f_898287c, 1, P.info_story);
    r->gap = MF(P.info_story) * 0.001f;
    /* +4 a rival i's, +0xc + i whether it is there (CSeq: its car on the road and not out) */
    for (int i = 0; i < 3; ++i) {
      r->story_gap[i] = MF(P.info_story + 4 * i) * 0.001f;
      r->story_on[i] = M8(P.info_story + 0xc + i);
    }
    /* clRaceThread::finiStory: won = rank 0 && !TimeUp && !Retire. GetPlayResult only fills
     * the result for 3/6/8 (mask 0x148); on 10 (Suspend: the episode's distance run without a
     * Defeat) the workspace keeps initRace's rank 0, so surviving the distance is a win. */
    if (st == 10) r->won = 1;
    else if (st == 3 || st == 6) r->won = 0;
    else if (st == 8) {
      call(f_898317c, 1, P.result);
      r->won = M32(P.result) == 0;
    }
  }
}

int car_title_list(const unsigned char *data, int n, int out[3]) {
  /* the image holds youTitle's tables; it is in once any race has been */
  if (!rc_mem && rc_load_image(P.image) != 0) return 0;
  /* above the stack call() starts at: _TitleData_ there, then the 3 slots */
  uint32_t td = RC_STACK_TOP - 0x800, list = RC_STACK_TOP - 0x40;
  memset(VA(td), 0, 0x7c0);
  memcpy(VA(td), data, (size_t)(n < 0x7c0 ? n : 0x7c0));
  call(f_858d8e8, 2, td, list);
  int got = 0;
  for (int i = 0; i < 3; ++i) {
    out[i] = (int)M32(list + 4 * (uint32_t)i);
    got += out[i] != 0;
  }
  return got;
}

const char *car_jct_block_name(int id) {
  if (id <= 0 || id >= CAR_JCT_BLOCKS) return NULL;
  const char *name = (const char *)VA(M32(JCT_BLOCK_NAMES + 4 * (uint32_t)id));
  /* a few carry the scene file's prefix ("yazirushi_all:..."); no strchr in the patched SDK */
  for (const char *p = name; *p; ++p)
    if (*p == ':') return p + 1;
  return name;
}

/* What the traffic shows this frame: each car COtherGen has on the road and, in a versus, the
 * other machines' share (CLxOtherGen: they drive it, it comes in their packets, this machine
 * hits it), as CSeq::GetInfoOtherCarDisp asks one then the other. */
static void read_traffic(car_state *c) {
  /* In a versus the other machines' share of the traffic comes in their packets, and the cars of it stand still while the packets are held up (a stall, a run of lost ones) and then
   * jump metres to where they are, a stream of cars going stop and go. What is drawn goes on at the speed they had while they stand, and is drawn toward where they are, a quarter of the
   * way a step, when the packets come back (the hit code is not touched: this is the picture only). MT3DX_TSMOOTH=0 turns it off, for a comparison. Display units, 0.1 m. */
  static struct {
    float src[2], out[2], vel[2];
    int frozen;
  } T[CAR_TRAFFIC];
  static unsigned tseen, tnow; /* (the slots shown on the last step, on this one) */
  static int jumps, shown_jumps, steps, logged, ts_on = -1;
  static float jmax;
  if (ts_on < 0) ts_on = getenv("MT3DX_TSMOOTH") ? atoi(getenv("MT3DX_TSMOOTH")) != 0 : 1;
  c->n_traffic = 0;
  tnow = 0;
  for (int i = 0; P.gen && i < CAR_TRAFFIC; i++) {
    if (!(call(f_89825a8, 3, (uint32_t)(TRAFFIC_SLOT0 + i), P.disp, 0u) & 0xff)) continue;
    car_traffic *t = &c->traffic[c->n_traffic++];
    t->model = P.traffic_model[i];
    memcpy(t->body, VA(P.disp + 4), sizeof t->body);
    if (!P.vs_race) continue;
    const float sx = t->body[12], sz = t->body[14];
    float ox = sx, oz = sz;
    if (tseen >> i & 1u) {
      const float dx = sx - T[i].src[0], dz = sz - T[i].src[1], d = sqrtf(dx * dx + dz * dz), v = sqrtf(T[i].vel[0] * T[i].vel[0] + T[i].vel[1] * T[i].vel[1]);
      if (d > 30.0f) { /* (3 m in a step: counted, as the source's own) */
        jumps++;
        if (d > jmax) jmax = d;
        if (logged++ < 80) NETLOG("versus traffic jump: slot %d moved %.1f m in one step (now %.0f %.0f)\n", i, (double)d * 0.1, (double)sx, (double)sz);
      }
      if (ts_on) {
        const float cx = T[i].out[0] + T[i].vel[0], cz = T[i].out[1] + T[i].vel[1]; /* (where it is drawn, gone on by its speed) */
        if (v > 2.0f && d < 0.15f * v && T[i].frozen < 24) { /* (it moved less than a seventh of what it does: held, not stopped; at most 0.4 s of that) */
          ox = cx, oz = cz;
          T[i].frozen++;
          T[i].vel[0] *= 0.98f, T[i].vel[1] *= 0.98f;
        } else {
          if (!T[i].frozen && d < 4.0f * v + 20.0f) T[i].vel[0] += 0.3f * (dx - T[i].vel[0]), T[i].vel[1] += 0.3f * (dz - T[i].vel[1]); /* (its speed, as it moves ordinarily) */
          const float ex = sx - cx, ez = sz - cz;
          if (ex * ex + ez * ez < 150.0f * 150.0f) ox = cx + 0.25f * ex, oz = cz + 0.25f * ez; /* (past 15 m it is another car, or a stall too long to bridge: no blend) */
          T[i].frozen = 0;
        }
        const float ddx = ox - T[i].out[0], ddz = oz - T[i].out[1];
        if (sqrtf(ddx * ddx + ddz * ddz) > 30.0f) shown_jumps++;
        t->body[12] = ox, t->body[14] = oz;
      } else {
        T[i].vel[0] = dx, T[i].vel[1] = dz;
      }
    } else {
      T[i].vel[0] = T[i].vel[1] = 0.0f, T[i].frozen = 0;
    }
    T[i].src[0] = sx, T[i].src[1] = sz, T[i].out[0] = ox, T[i].out[1] = oz;
    tnow |= 1u << i;
  }
  if (P.vs_race) {
    tseen = tnow;
    if (++steps >= 600) {
      if (jumps || shown_jumps) NETLOG("versus traffic: %d source jumps, %d drawn jumps (over 3 m in a step) in 600 steps; the largest source jump %.1f m\n", jumps, shown_jumps, (double)jmax * 0.1);
      steps = jumps = shown_jumps = 0, jmax = 0;
    }
  }
}

/* The rivals (CSeq::GetInfoRivalCarDisp, rivals 1..): off the road (out of reach) it says so. */
static void read_rival(car_state *c) {
  for (int i = 0; i < CAR_RIVALS; ++i) {
    c->rival_shown[i] = (i < P.n_rival || i < P.vs_race || (P.ten_stage && i == 0)) && (call(f_8983488, 3, (uint32_t)i + 1u, P.disp, P.eff) & 0xff);
    if (P.vs_race && i < P.vs_n && (link_gone >> P.vs[i].pcb & 1u)) c->rival_shown[i] = 0; /* (a machine that left: its car off the road) */
    c->rival_effect[i].spark = 0;
    if (c->rival_shown[i]) {
      read_effect(&c->rival_effect[i]);
      memcpy(c->rival_body[i], VA(P.disp + 4), sizeof c->rival_body[i]);
      read_tyres(c->rival_tyre[i], c->rival_tyre_rot[i]);
      c->rival_brake[i] = M8(P.disp + 0x144);
      c->rival_spot[i] = M8(P.disp + 0x146) == 1;
      if (P.vs_race && i < P.vs_n) c->rival_spot[i] = !net_flash_of(P.vs[i].pcb); /* (a machine's own headlights as it sends them: the rival code shows them always) */
      if (P.vs_race && i < P.vs_n) { /* (measuring: the shown car's step against the last one's: a jerk of more than 0.6 m is a jump the player sees) */
        static float rp_[CAR_RIVALS][3], rd_[CAR_RIVALS][3];
        static int rok[CAR_RIVALS], rlog_, rjumps[CAR_RIVALS], rsteps, rtotal;
        static float rmax[CAR_RIVALS];
        const float *b = c->rival_body[i];
        float d[3] = { b[12] - rp_[i][0], 0.0f, b[14] - rp_[i][2] };
        if (rok[i] == 2) {
          float jx = d[0] - rd_[i][0], jz = d[2] - rd_[i][2], j = sqrtf(jx * jx + jz * jz);
          if (j > 5.0f && rtotal > 900) {
            rjumps[i]++;
            if (j > rmax[i]) rmax[i] = j;
            if (rlog_++ < 100) NETLOG("versus rival jump: pcb %d jerked %.2f m in one step (step %.2f, was %.2f)\n", P.vs[i].pcb, (double)j * 0.1, (double)sqrtf(d[0] * d[0] + d[2] * d[2]) * 0.1, (double)sqrtf(rd_[i][0] * rd_[i][0] + rd_[i][2] * rd_[i][2]));
          }
        }
        rd_[i][0] = d[0], rd_[i][2] = d[2], rp_[i][0] = b[12], rp_[i][2] = b[14];
        if (rok[i] < 2) rok[i]++;
        if (i == 0 && (rtotal++, ++rsteps >= 600)) {
          for (int k = 0; k < CAR_RIVALS; ++k)
            if (rjumps[k]) NETLOG("versus rival: slot %d: %d jerks over 600 steps, the largest %.2f m\n", k, rjumps[k], (double)rmax[k] * 0.1);
          memset(rjumps, 0, sizeof rjumps), memset(rmax, 0, sizeof rmax), rsteps = 0;
        }
      }
    }
  }
}

/* The realtime movies: clRaceThread::initStory makes a clMovieSystem (0x1c1fc bytes) and Inits
 * it with the movie, the player's car and the rivals'; its CMovieEditWraps runs CSeq in its
 * movie mode, each car a yt::CGhost driven along its recorded trail (data/rtmovie/trail). Each
 * frame clRaceThread::race runs CSeq::Exec, then UpDate(frame), GetRTMovieCamera and
 * GetRTMovieCarDisp for cars 0..3; the frame goes on by 1 up to the length Init gave. */
#define MOVIE_SYSTEM_SIZE 0x1c1fcu
static uint32_t movie;

float car_movie_start(car_state *c, int id, const car_spec *player, const car_rival *rivals, int n) {
  P.ready = 0;
  movie = 0;
  if (rc_load_image(P.image) != 0) return 0;
  rc_reset_heap();
  P.spec = player ? *player : CAR_SPEC_FD3S;
  car_set_rivals(rivals, n);
  rc_on_hook = on_hook;
  rc_on_other_setup = on_other_setup;
  P.ten_stage = 0;
  uint32_t rival_type[CAR_RIVALS] = { 0 };
  for (int i = 0; i < P.n_rival; ++i) rival_type[i] = P.rival[i].type;
  uint32_t ms = rc_alloc(MOVIE_SYSTEM_SIZE);
  call(f_89fe784, 2, ms, 1u);
  call(f_89fd9d0, 6, ms, (uint32_t)id, P.spec.type, rival_type[0], rival_type[1], rival_type[2]);
  float len = (float)ST(0);
  fpop();
  call(f_89a2584, 1, fbits(1.0f / 60.0f));
  call(f_89a2548, 1, fbits(1.0f));
  P.m = M32(MACHINE_PTR);
  P.gen = M32(OTHER_GEN_PTR);
  P.gin = rc_alloc(0x40);
  P.gin_ptr = rc_alloc(4);
  W32(P.gin_ptr, P.gin);
  W32(P.gin + 0x14, 0xffffffffu);
  P.disp = rc_alloc(0x260);
  P.cam = rc_alloc(0x100);
  P.mirror = 0;
  movie = ms;
  memset(c, 0, sizeof *c);
  return len;
}

int car_movie_update(car_state *c, float frame) {
  if (!movie) return 0;
  call(f_8983f64, 1, P.gin_ptr);
  call(f_89fe890, 2, movie, fbits(frame));
  if (call(f_89fde9c, 2, movie, P.cam) & 0xff) {
    memcpy(c->cam_view, VA(P.cam), sizeof c->cam_view);
    const float *V = c->cam_view;
    for (int j = 0; j < 3; j++) c->cam_pos[j] = -(V[12] * V[4 * j] + V[13] * V[4 * j + 1] + V[14] * V[4 * j + 2]);
    c->cam_fov = MF(P.cam + 0x40);
    c->cam_near = MF(P.cam + 0x44);
    c->cam_blur = 0;
  }
  int shown = 0;
  for (int i = 0; i <= CAR_RIVALS; ++i) {
    int on = call(f_89fdfe8, 3, movie, (uint32_t)i, P.disp) & 0xff;
    shown |= on << i;
    if (!i) {
      if (!on) continue;
      memcpy(c->body, VA(P.disp + 4), sizeof c->body);
      read_tyres(c->tyre, c->tyre_rot);
      c->brake = M8(P.disp + 0x144);
      c->spot = M8(P.disp + 0x146) == 1;
      for (int j = 0; j < 3; j++) c->pos[j] = c->body[12 + j];
      continue;
    }
    c->rival_shown[i - 1] = on;
    if (on) {
      memcpy(c->rival_body[i - 1], VA(P.disp + 4), sizeof c->rival_body[i - 1]);
      read_tyres(c->rival_tyre[i - 1], c->rival_tyre_rot[i - 1]);
      c->rival_brake[i - 1] = M8(P.disp + 0x144);
      c->rival_spot[i - 1] = M8(P.disp + 0x146) == 1;
    }
  }
  c->n_traffic = 0;
  return shown;
}

void car_movie_end(void) { movie = 0; }

/* Cut::clCutIn (0x14b4 bytes): the ctor, Init(select, loop), Update */
#define CUTIN_SIZE 0x14b4u
static uint32_t cutin;
static car_cutin_cb cutin_cb;

static void on_cutin_speak(uint32_t pos, uint32_t tag, int flash, int x) {
  /* tag_SCatinDisp: eChar, expression, the line's anime, the flash's, the string (rc_utf8_line's) */
  uint32_t str = M32(tag + 0x10);
  int row = str ? (int)M32(str) - 1 : -1;
  if (cutin_cb.speak && row >= 0)
    cutin_cb.speak((int)pos, row, (int)M32(tag + 8), (int)M32(tag + 0xc), flash & 0xff, x);
}

static void on_cutin_end(uint32_t pos) {
  if (cutin_cb.end) cutin_cb.end((int)pos);
}

const char *car_cutin_start(int select, int loop, const car_cutin_cb *cb) {
  cutin = 0;
  if (!P.ready || select < 0) return NULL;
  cutin_cb = *cb;
  rc_on_cutin_speak = on_cutin_speak;
  rc_on_cutin_end = on_cutin_end;
  rc_cutin_mode = 2;
  rc_cutin_lines = 0;
  rc_last_path[0] = 0;
  uint32_t c = rc_alloc(CUTIN_SIZE);
  call(f_85b15a8, 1, c);
  /* Init sets up only before the race's CSeq::Init (GetCutInInfoPt still none); its mode
   * (+0: 2 the story's) back to 0 when the file could not be read */
  uint8_t on = M8(CUTIN_ON);
  *(uint8_t *)VA(CUTIN_ON) = 0;
  call(f_85b16a4, 3, c, (uint32_t)select, (uint32_t)loop);
  *(uint8_t *)VA(CUTIN_ON) = on;
  if (M32(c) != 2 || !rc_last_path[0]) return NULL;
  cutin = c;
  const char *slash = strrchr(rc_last_path, '/');
  return slash ? slash + 1 : rc_last_path;
}

void car_cutin_update(void) {
  if (cutin && P.ready) call(f_85b3754, 1, cutin);
}

void car_cutin_stop(void) { cutin = 0; }

/* yt::CSeq::End (clRaceThread::finiStory's): among the rest it saves the race's yt::SCarryover
 * (rc_runtime keeps /tmp/ytCarryover.txt in memory), which the next race of an episode whose
 * series carries over (ytConfigStory's Suspend_Carryover ... true: the B races) starts from */
void car_story_end(int first) {
  if (first) rc_carry_clear();
  else if (P.ready && P.has_rival) call(f_8986878, 0);
}

/* The top bar's reset key (a port extra: the arcade has no such key): the car put back on the
 * road where it is, facing the course in the start's lane, the way CSeq::Exec's 10-in-a-row
 * warp places it (0x898644c: yt::CSlit::GetFirstPoint at an odometer reading and a lateral
 * offset, CSeq::StartOfsAry[1] the start's; yt::CMachine::SetFirstPos, which sets the body,
 * the tyres, the gear for the speed, the hit and the camera effects, its speed the warp's
 * 120 km/h at most here, the car's own forward speed below that). */
#define START_OFS RCA(9802920)     /* yt::CSeq::StartOfsAry: [1] the start's lateral offset */
#define WARP_SPEED 33333.333f    /* SetFirstPos's speed for the warp (mm/s: 300 km/h) */
static int hits; /* car_hits */
#define SPARK_CLEAR 30 /* frames without the scrape before its next start is a hit */
void car_hits(int on) { hits = on != 0; }

void car_reset(car_state *c) {
  if (!P.ready) return;
  c->vel_ok = 0; /* (the warp is no hit) */
  uint32_t slit = P.m + 0x7b4u;
  float odo = MF(slit + 0x1bc), speed = fminf(fmaxf(c->speed * 1000.0f, 0.0f), getenv("MT3DX_WARPKEEP") ? 1e9f : WARP_SPEED); /* (MT3DX_WARPKEEP: the car's own speed, as a cutscene's restart keeps it) */
  call(f_89c50c0, 4, P.first_pt, slit, fbits(odo), M32(START_OFS + 4));
  call(f_89ba05c, 3, P.m, P.first_pt, fbits(speed));
  if (P.manual) { /* the lever to the gear SetFirstPos chose (CTransmission::SetFirst) */
    int g = (int)M32(P.m + M_ENGINE + 0x3c);
    P.lever = g < 1 ? 0 : g - 1 > P.top_lever ? P.top_lever : g - 1;
  }
  read_body(c); /* car_update's displacement from here */
  c->steer = 0;
}

static void first_frame(car_state *c) {
  memset(c, 0, sizeof *c);
  /* the tyres' radii (STireDimension's, mm) in display units: the player's, then the rivals' */
  c->tyre_radius[0] = P.spec.tyres[1] * 0.01f;
  for (int i = 0; i < CAR_RIVALS; ++i)
    c->tyre_radius[1 + i] = (P.vs_on && i < P.vs_n ? P.vs[i].tyres[1] : i < P.n_rival ? P.rival[i].tyres[1] : 0.0f) * 0.01f;
  /* the rolling start is at about 120 km/h: second gear */
  P.lever = 1;
  read_body(c);
  read_camera(c);
  read_jct_blocks(c);
  read_traffic(c);
  read_rival(c);
#ifdef WMMT_EGL
  if (getenv("WMMT_TA_GOAL")) { /* test: the checkpoints and the goal (car_start) within this many m */
    uint32_t tk = M32(TIME_KEEPER_PTR);
    int n = tk ? (int)M32(tk + TK_COUNT) : 0;
    float len = 1000.0f * (float)atof(getenv("WMMT_TA_GOAL"));
    for (int k = 1; k < n; ++k) WF(M32(tk + TK_ODO) + 4u * (uint32_t)k, COURSES[P.course].dist + len * (float)k / (float)(n - 1));
  }
  if (getenv("WMMT_TA_TIME")) WF(RCA(9802a6c), (float)atof(getenv("WMMT_TA_TIME"))); /* test: the time to start with */
#endif
  if (vs_time_km > 13) { /* a versus' time: the arcade's 360 s are for its 13 km (a 20 km race at full pace ran out); the same pace for the km chosen */
    const float t = 360.0f * vs_time_km / 13.0f;
    WF(RCA(9802a6c), t);
    fprintf(stderr, "versus: %d km, the time to start with %.0f s (the arcade's 360 is for 13)\n", (int)vs_time_km, (double)t);
  }
  read_race(&c->race);
  c->manual = P.manual;
  c->gear = P.manual ? 2 : 1;
  { /* the gear SetFirstPos chose (CTransmission::SetFirst): the one the countdown asks for */
    int g = (int)M32(P.m + M_ENGINE + 0x3c);
    if (P.spec.gears <= 5 && g > 2) g = 2; /* (3rd is out of the power band of a 4 or 5 speed car: the rolling start asks for 2nd) */
    c->start_gear = g >= 1 && g <= 6 ? g : 2;
    if (P.manual) P.lever = c->start_gear - 1 > P.top_lever ? P.top_lever : c->start_gear - 1; /* (the lever starts where the engine's gear is: no shift of its own at the start) */
    if (getenv("MT3DX_LOG")) fprintf(stderr, "start gear: engine says %d, shown %d\n", g, c->start_gear);
  }
  c->on_ground = 1;
}

void car_set_manual(car_state *c, int manual) {
  P.manual = manual != 0;
  c->manual = P.manual;
  if (!P.ready) return;
  /* the flag CTransmission::TransOn reads every frame (a reset starts with P.manual's) */
  W32(P.m + M_ENGINE + 0x34, P.manual ? 0u : 1u);
  /* the lever takes over from the gear the automatic was in */
  if (P.manual) {
    int g = (int)M32(P.m + M_ENGINE + 0x3c);
    P.lever = g < 1 ? 0 : g - 1 > P.top_lever ? P.top_lever : g - 1;
  }
}

float car_kmh(const car_state *c) { return fabsf(c->speed) * 3.6f; }

/* car_phys: CMachine's tyre input (+0xaf8: CAngle at +0, the slip ratio at +4 of the 12 bytes' second word, the load at +8, as CTire::GetTireInput
 * returns them), the suspension's load ratio (+0xae4), and the tyre (+0x2b4) asked again for its forces at that input and over every slip angle */
#define M_TIRE_INPUT 0xaf8u
#define M_LOAD_RATIO 0xae4u
#define M_TIRE 0x2b4u
#define M_SIM_IN 0xb04u /* the CIOBase copy PreDrive makes: wheel (+ to the left), gas, brake */
static int phys_dbg;
void car_phys_debug(int on) { phys_dbg = on != 0; }

static void read_phys(car_state *c, const float v[3], const float pv[3], int have_prev) {
  car_phys *p = &c->phys;
  const float *M = c->body;
  p->fwd = c->speed;
  p->lat = -(v[0] * M[0] + v[1] * M[1] + v[2] * M[2]); /* the body's +x is the car's left */
  p->slip_body = fabsf(p->fwd) > 1.0f ? atan2f(p->lat, fabsf(p->fwd)) : 0.0f;
  static float last_yaw;
  float dy = c->yaw - last_yaw;
  if (!isfinite(dy)) dy = 0.0f; /* (the loops below must end) */
  while (dy > 3.14159265f) dy -= 6.2831853f;
  while (dy < -3.14159265f) dy += 6.2831853f;
  last_yaw = c->yaw;
  p->yaw_rate = have_prev ? -dy * 60.0f : 0.0f; /* the heading grows turning left */
  if (have_prev) {
    float a[3] = { (v[0] - pv[0]) * 60.0f, (v[1] - pv[1]) * 60.0f, (v[2] - pv[2]) * 60.0f };
    float gl = (a[0] * M[8] + a[1] * M[9] + a[2] * M[10]) / 9.81f;
    float gs = -(a[0] * M[0] + a[1] * M[1] + a[2] * M[2]) / 9.81f;
    p->g_long += (gl - p->g_long) * 0.35f;
    p->g_lat += (gs - p->g_lat) * 0.35f;
  }
  p->tyre_angle = (float)(int16_t)M16(P.m + M_TIRE_INPUT) * (3.14159265f / 32768.0f);
  p->tyre_ratio = MF(P.m + M_TIRE_INPUT + 4);
  p->tyre_load = MF(P.m + M_TIRE_INPUT + 8);
  p->load_ratio = MF(P.m + M_LOAD_RATIO);
  p->in_steer = -MF(P.m + M_SIM_IN), p->in_gas = MF(P.m + M_SIM_IN + 4), p->in_brake = MF(P.m + M_SIM_IN + 8);
  if (phys_dbg) {
    /* above the stack call() starts at (as car_title_list's): the 12-byte result and a copy of the input to turn */
    uint32_t out = RC_STACK_TOP - 0x100, in2 = RC_STACK_TOP - 0xe0;
    call(f_89ce5a4, 3, out, P.m + M_TIRE, P.m + M_TIRE_INPUT);
    p->f_lat = MF(out), p->f_long = MF(out + 8);
    memcpy(VA(in2), VA(P.m + M_TIRE_INPUT), 12);
    float pa = 0, pb = 0, pang = 0;
    uint32_t ang0 = M32(in2) & 0xffffu;
    for (int k = 0; k <= 32; ++k) { /* the sideways force over every slip angle, 0 .. 90 degrees (a CAngle: 32768 is pi), at this slip ratio */
      W32(in2, (M32(in2) & ~0xffffu) | (uint32_t)(k * 512));
      call(f_89ce5a4, 3, out, P.m + M_TIRE, in2);
      float fa = fabsf(MF(out));
      if (fa > pa) pa = fa, pang = (float)(k * 512) * (3.14159265f / 32768.0f);
    }
    W32(in2, (M32(in2) & ~0xffffu) | ang0);
    for (int k = -32; k <= 32; ++k) { /* the force along the car over every slip ratio, -1 .. 1, at this slip angle */
      WF(in2 + 4, (float)k / 32.0f);
      call(f_89ce5a4, 3, out, P.m + M_TIRE, in2);
      pb = fmaxf(pb, fabsf(MF(out + 8)));
    }
    p->peak_lat = pa, p->peak_long = pb, p->peak_angle = pang;
  }
  static int logged = -1, n;
  if (logged < 0) logged = getenv("MT3DX_PHYSLOG") != NULL; /* tests: a line every 6th frame, to stderr */
  if (logged && ++n % 6 == 0)
    fprintf(stderr, "phys steer %+.2f kmh %.0f fwd %.1f lat %+.2f slip %+.1f yaw %+.1f g %+.2f %+.2f tyre %+.1f ratio %+.3f load %.0f sus %.2f side %+.0f/%.0f long %+.0f/%.0f gin %+.2f io %+.3f %+.3f %+.3f\n",
            c->steer, fabsf(p->fwd) * 3.6f, p->fwd, p->lat, p->slip_body * 57.2958f, p->yaw_rate * 57.2958f, p->g_lat, p->g_long,
            p->tyre_angle * 57.2958f, p->tyre_ratio, p->tyre_load, p->load_ratio, p->f_lat, p->peak_lat, p->f_long, p->peak_long, MF(P.gin), MF(P.m + 0xb04), MF(P.m + 0xb08), MF(P.m + 0xb0c));
}

static int filt_on, filt_smooth, filt_counter, btn_steer = 10;
void car_set_button_steer(int v) { btn_steer = v < 1 ? 1 : v > 10 ? 10 : v; }
void car_set_filter(int on, int smooth, int counter) {
  filt_on = on != 0;
  filt_smooth = smooth < 0 ? 0 : smooth > 20 ? 20 : smooth;
  filt_counter = counter < 0 ? 0 : counter > 20 ? 20 : counter;
}

void car_update(car_state *c, const car_input *in) {
  if (!P.ready) return;
  /* digital buttons: the arcade expects a wheel, so stand in for the driver's hands.
   * Turn in at a steady rate and centre faster, to full lock at any speed as the cabinet's
   * wheel goes; the tilt is a wheel already, followed closely */
  float target = in->steer;
  c->steer_in = target;
  {
    /* the buttons (keys, the D-pad): the push builds up at BUTTON STEERING SPEED (1..10: turn in at 0.6 x it, centre at 1.6 x it, in lock-widths a
     * second; 10 is straight to full lock as before), before the filter or the direct rate; an analog input goes through as it is */
    static float held;
    if (in->analog || btn_steer >= 10) {
      held = target;
    } else {
      float rate = fabsf(target) > fabsf(held) && target * held >= 0 ? 0.6f * (float)btn_steer : 1.6f * (float)btn_steer;
      float ds = target - held;
      if (ds > rate / 60.0f) ds = rate / 60.0f;
      if (ds < -rate / 60.0f) ds = -rate / 60.0f;
      held += ds;
    }
    target = held;
  }
  if (filt_on) {
    /* filtered: first the counter-steer mapping (while the car slides, a push towards the way it is going is bent towards its cube); then a first-order
     *
     * lag, its time constant from SMOOTH */
    float slip = c->phys.slip_body, sa = fabsf(slip), x = target;
    if (filt_counter > 0 && x * slip > 0.0f) {
      float s = (sa - 0.07f) / 0.28f;
      s = s < 0.0f ? 0.0f : s > 1.0f ? 1.0f : s;
      float m = (float)filt_counter * 0.05f * s; /* (counter / 10 of the way, the counter in half steps) */
      x = x * (1.0f - m) + m * x * x * x;
    }
    float ta = (float)filt_smooth * 0.5f * 0.03f, y = c->steer;
    float ny = ta <= 1e-4f ? x : y + (x - y) * (1.0f - expf(-(1.0f / 60.0f) / ta));
    c->steer = ny;
  } else {
    float rate = in->analog ? 12.0f : fabsf(target) > fabsf(c->steer) && target * c->steer >= 0 ? 3.0f : 8.0f;
    float ds = target - c->steer;
    if (ds > rate / 60.0f) ds = rate / 60.0f;
    if (ds < -rate / 60.0f) ds = -rate / 60.0f;
    c->steer += ds;
  }

  /* clGameInput for CIO::Drive: wheel, pedals and the shift lever (gear - 1, -1 = neutral).
   * With AT the arcade ignores the lever. */
  if (P.manual) {
    if (in->gear > 0) P.lever = in->gear - 1 > P.top_lever ? P.top_lever : in->gear - 1;
    if (in->shift_up && P.lever < P.top_lever) P.lever++;
    if (in->shift_down && P.lever > -1) P.lever--;
  }
  WF(P.gin, c->steer);
#ifdef WMMT_EGL
  if (getenv("WMMT_PILOT") || AUTOP) { /* test: the arcade's own pilot (CMcPilot) steers, flat out (and in a paused versus race) */
    uint32_t pilot = M32(RCA(98028dc));
    W32(pilot, M32(RCA(9802924)));
    W32(pilot + 4, M32(RCA(9802928)));
    call(f_89bcca8, 2, pilot, P.m);
    WF(P.gin, -MF(pilot + 0x14));
    if (getenv("WMMT_PILOT_LOG")) { /* test: what the pilot reads and answers, every 2 s */
      static int n;
      if (++n % 120 == 1)
        fprintf(stderr, "pilot: in %g %g, out +0x10 %g +0x14 %g +0x18 %g +0x1c %g\n", MF(pilot), MF(pilot + 4), MF(pilot + 0x10), MF(pilot + 0x14), MF(pilot + 0x18), MF(pilot + 0x1c));
    }
    if (getenv("WMMT_PILOT_BIAS")) WF(P.gin, MF(P.gin) + (float)atof(getenv("WMMT_PILOT_BIAS"))); /* test: a steady steering offset, to hug a lane */
  }
#endif
  if (AUTOP) WF(P.gin + 4, c->speed < AUTOP_V ? 1.0f : 0.0f), WF(P.gin + 8, 0.0f); /* (held at the speed it had) */
  else WF(P.gin + 4, in->throttle), WF(P.gin + 8, in->brake);
  W32(P.gin + 0x14, P.manual ? (uint32_t)P.lever : 0xffffffffu);
  /* the arcade frame: the traffic's AI and life, the car, what everything hit, the
   * checkpoints and the time, the sounds' cues */
  call(f_8983f64, 1, P.gin_ptr);
  /* the headlights' flash: the machine's +0x3b5 (clCarDisp's spot light and flares are on while it is 0; the packet's byte 0x41 is its inverse) is cleared inside the arcade
   * frame, so it is set after it, for the packet (lx_record) and the display (read_body) of this step */
  if (P.vs_race) W8(P.m + 0x3b5u, in->flash ? 1u : 0u), net_set_flash(in->flash);
  if (P.vs_race) lx_record();

  float prev[3] = { c->pos[0], c->pos[1], c->pos[2] };
  float prev_speed = c->speed;
  read_body(c);
  read_camera(c);
  read_jct_blocks(c);
  read_traffic(c);
  read_rival(c);
  rival_impacts(c);
  read_race(&c->race);
  read_se(&c->se);
  c->camfx[0] = MF(P.m + 0xb60 + 8), c->camfx[1] = MF(P.m + 0xb60 + 0xc);
  c->camfx[2] = MF(P.m + 0xb60 + 0x14), c->camfx[3] = MF(P.m + 0xb60 + 0x18);
  /* velocity from the frame's displacement (display units per frame -> m/s) */
  float v[3] = { (c->pos[0] - prev[0]) * 6.0f, (c->pos[1] - prev[1]) * 6.0f, (c->pos[2] - prev[2]) * 6.0f };
  const float *M = c->body;
  c->speed = v[0] * M[8] + v[1] * M[9] + v[2] * M[10];
  autop_speed = c->speed;
  float lateral = v[0] * M[0] + v[1] * M[1] + v[2] * M[2];

  c->rpm = MF(P.m + M_ENGINE + 8) * (60.0f / 6.2831853f);
  /* CEngine::GetRevRate: rpm over the engine's rev limit (its parameters' +0x8c) */
  float limit = MF(M32(P.m + M_ENGINE) + 0x8c);
  c->rev_rate = limit > 0 ? fminf(1.0f, fmaxf(0.0f, c->rpm / limit)) : 0;
  c->boost = MF(P.m + M_TURBO + 4);
  {
    static float pvel[3];
    read_phys(c, v, pvel, phys_have);
    memcpy(pvel, v, sizeof pvel);
    phys_have = 1;
  }
  int gear = (int)M32(P.m + M_ENGINE + 0x3c);
  /* a manual car while a countdown runs (the 10-in-a-row's restart after a beaten rival: the engine's gear is the rolling start's 2nd while the
   * driver's lever, the port's own, is still where it was, 5th): the lever's gear is the one that is in once the race runs, so it is the one shown
   * (the display dropped to 2 and jumped back at the start, issue 11) */
  if (P.manual && c->race.countdown > 0.0f && P.lever >= 0 && P.lever != gear - 1) gear = P.lever + 1;
  { /* A cutscene that goes on at the car's speed puts the car back at its first point (CMachine::SetFirstPos), and CTransmission::SetFirst picks the rolling start's gear
     * for it, 2nd, at whatever speed the car has: the engine over-revs in a "money shift" and the gear shown drops from 4th to 2nd (issue 41). A fall of two gears or more in
     * one step at a speed that is kept, not a shift the driver asked for (the manual's lever below the new gear, no gear key pressed; the automatic does not skip gears
     * by itself), is that: the gear it was in is put back. */
    const int prev_gear = c->gear;
    if (prev_gear >= 3 && c->race.play_state == 2 && c->race.countdown <= 0.0f && !GK.active && prev_gear - gear >= 2 && fabsf(prev_speed) > 10.0f && fabsf(c->speed) > 0.9f * fabsf(prev_speed) &&
        (!P.manual || (!in->gear && !in->shift_down && gear - 1 != P.lever))) {
      fprintf(stderr, "gear: fell from %d to %d in one step at %.0f km/h with no shift asked for (a cutscene's restart): put back\n", prev_gear, gear, (double)fabsf(c->speed) * 3.6);
      W32(P.m + M_ENGINE + 0x3c, (uint32_t)prev_gear);
      gear = prev_gear;
      if (P.manual) P.lever = prev_gear - 1 > P.top_lever ? P.top_lever : prev_gear - 1;
    }
  }
  { /* the gear kept through a restart (see GK) */
    const int running = c->race.play_state == 2 && c->race.countdown <= 0.0f;
    if (CARRY && CARRY_LIVE) { /* the story's second race: the gear the first one ended in, from the first step, whether or not a countdown runs */
      if (gear != CARRY) {
        fprintf(stderr, "gear: a story select's next race starts in %d, the first one ended in %d (the countdown %.1f, play state %d): keeping %d\n", gear, CARRY, (double)c->race.countdown, c->race.play_state, CARRY);
        W32(P.m + M_ENGINE + 0x3c, (uint32_t)CARRY), gear = CARRY;
        if (P.manual) P.lever = CARRY - 1 > P.top_lever ? P.top_lever : CARRY - 1;
      }
    }
    if (running) GK.running = 1, GK.keep = gear, GK.active = 0, GK.forced = 0, GK.spd = fabsf(c->speed), CARRY = CARRY_LIVE ? 0 : CARRY;
    else if (GK.running && c->race.countdown > 0.0f && c->race.play_state <= 2 && GK.keep >= 1) {
      GK.running = 0, GK.active = 1, GK.forced = 0;
      fprintf(stderr, "gear: the race went back to a countdown in gear %d (the engine's now %d, the lever %d): keeping %d\n", GK.keep, gear, P.lever + 1, GK.keep);
    }
    /* A cutscene's restart goes on at the car's speed, so the gear it was in is still the right one; the 10-in-a-row's restart after a beaten rival is a rolling start at a
     * speed of its own, where the arcade's 2nd is the right gear and a kept 5th is money-shifted down at the lights (a car that slows a great deal). A car that is well
     * under the speed it had is that one: the arcade's gear goes back in. */
    if (GK.active && c->race.countdown > 0.0f && GK.spd > 15.0f && fabsf(c->speed) < 0.6f * GK.spd) {
      fprintf(stderr, "gear: not kept: the restart's speed %.0f km/h is well under the %.0f it had (a rolling start): the arcade's %d stays\n", (double)fabsf(c->speed) * 3.6, (double)GK.spd * 3.6, GK.forced ? GK.orig : gear);
      if (GK.forced && GK.orig >= 1) W32(P.m + M_ENGINE + 0x3c, (uint32_t)GK.orig), gear = GK.orig;
      GK.active = 0, GK.keep = 0;
    }
    if (GK.active) {
      if (c->race.countdown > 0.0f) {
        if (!GK.forced) GK.orig = (int)M32(P.m + M_ENGINE + 0x3c), GK.forced = 1;
        if (gear != GK.keep) W32(P.m + M_ENGINE + 0x3c, (uint32_t)GK.keep), gear = GK.keep;
        if (P.manual) P.lever = GK.keep - 1 > P.top_lever ? P.top_lever : GK.keep - 1;
      } else {
        GK.active = 0;
      }
    }
  }
  if (gear != c->gear && c->gear) {
    c->shift_event = gear > c->gear ? 1 : -1;
    if (getenv("MT3DX_LOG")) fprintf(stderr, "gear: %d -> %d at %.0f km/h (lever %d, play state %d)\n", c->gear, gear, (double)fabsf(c->speed) * 3.6, P.lever + 1, c->race.play_state);
  }
  c->gear = gear;
  bb_step(c);

  /* effect cues: squeal from sideways speed, impact from a sudden loss of speed */
  float squeal = fminf(1.0f, fmaxf(0.0f, (fabsf(lateral) - 1.5f) / 5.0f));
  if (in->brake > 0.5f && fabsf(c->speed) > 15.0f) squeal = fmaxf(squeal, 0.7f * in->brake);
  c->slip += (squeal - c->slip) * 0.15f;
  float lost = fabsf(prev_speed) - fabsf(c->speed);
  c->wall_hit = fmaxf(0.0f, c->wall_hit - 1.0f / 60.0f);
  if (hits) {
    /* the frame's change of velocity across the ground, past what the tyres do (about 1.5 g,
     * 0.25 m/s a frame; a hard corner flat out comes to 0.4 on the Mi 6's log). Sliding along
     * a wall at 120 km/h bounces 0.5..0.7 m/s a frame: the wall's loop; a crash (snd_update's
     * from impact 6) from 0.8 m/s, a car's or a wall's hit */
    float dx = v[0] - c->vel[0], dz = v[2] - c->vel[2], dv = sqrtf(dx * dx + dz * dz);
    if (c->vel_ok && dv > 0.5f) c->wall_hit = 0.3f;
    if (c->vel_ok && dv > 0.8f) c->impact = fmaxf(c->impact, dv * 12.0f);
    /* the arcade's scrape: the wall's loop while it lasts, a light crash as it starts after
     * half a second clear (it flickers on and off along a wall) */
    if (c->effect.spark > 0.0f) {
      c->wall_hit = fmaxf(c->wall_hit, 0.2f);
      if (!c->sparked) c->impact = fmaxf(c->impact, 7.0f);
    }
    c->sparked = c->effect.spark > 0.0f ? SPARK_CLEAR : c->sparked > 0 ? c->sparked - 1 : 0;
    memcpy(c->vel, v, sizeof c->vel);
    c->vel_ok = 1;
  } else if (lost > 0.4f) { /* beyond what the brakes do (about 1.5 g = 0.25 m/s per frame) */
    c->impact = fmaxf(c->impact, lost * 4.0f);
    c->wall_hit = 0.3f;
  }
}
