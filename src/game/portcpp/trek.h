/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
int SF_DebugMem_Check(void *,char *);

class GenericClass;
int IsValidObjectPointer(GenericClass *,char * =0, int=0, short=FALSE);

typedef char RotationType;

#define nlong(d) slong(d)

/*** Classes ***/

/* Port 2026-10-12: pre-standard compilers made "friend struct PingPacket"
   visible here; modern C++ needs an explicit forward declaration. */
struct PingPacket;

class CoordinateClass {
  /* holds standard x y coordinates -- longs */
 public:
  friend class GenericClass;
  friend class PlanetClass;
  friend class DesignedClass;
  friend class ShipClass;
  friend class MissileClass;
  friend class GameStateClass;
  friend struct PingPacket;
  CoordinateClass() {
    initialize();
  }
  CoordinateClass(slong c1, slong c2) {
    /* long constructor -- takes coordinates */
    initializeWith(c1,c2);
  }
  void initialize() {
    x=0;
    y=0;
  }
  void initializeWith(slong c1, slong c2) {
    /* long constructor -- takes coordinates */
    x = c1;
    y = c2;
  }
  //void diff(char[], CoordinateClass&);
  void operator+=(const CoordinateClass& c) {
    x+=c.x;
    y+=c.y;
  }
  void operator-=(const CoordinateClass& c) {
    x-=c.x;
    y-=c.y;
  }
  short operator==(const CoordinateClass& c) {
    return x == c.x && y == c.y;
  }
  CoordinateClass& operator-(const CoordinateClass& c) const {
    static CoordinateClass cc;
    cc.x = x - c.x;
    cc.y = y - c.y;
    return cc;
  }
  CoordinateClass& operator+(const CoordinateClass& c) {
    static CoordinateClass cc;
    cc.x = x + c.x;
    cc.y = y + c.y;
    return cc;
  }
  CoordinateClass& operator*(slong l) {
    static CoordinateClass cc;
    cc.x = l * x;
    cc.y = l * y;
    return cc;
  }
  CoordinateClass& operator/(slong l) {
    static CoordinateClass cc;
    cc.x = x / l;
    cc.y = y / l;
    return cc;
  }
  slong operator*(CoordinateClass c) { /* dot product */
    return x*c.x + y*c.y;
  }
  slong crossMag(CoordinateClass c) {
    /* return z component of cross product */
    return ((x/sub_scale)*c.y - (c.x/sub_scale)*y) * sub_scale;
  }
  CoordinateClass& operator*=(double f) {
    x = nlong(x*f);
    y = nlong(y*f);
    return *this;
  }
  CoordinateClass& operator/=(slong l) {
    x /= l;
    y /= l;
    return *this;
  }
  RotationType Heading(short numFrames) const {
    slong Atan2(slong,slong);
    return (RotationType)(((Atan2(y,x) + 270) * numFrames/360 ) % numFrames);
  }
  short IsInFiringArc(short& offset, slong offset1, slong offset2, 
		      short numFrames) {
    if (offset < offset1) 
      offset += numFrames;
    return (offset >= offset1 && offset <= offset2);
  }
  short AddSubjectToLimit(const CoordinateClass&, slong);
  ulong Mag(ulong range =dist_crit) const               /* return magnitude  */
  {
    /* return the magnitude of a Coordinate Class variable */
    /*   if the distance is greater than the critical distance dist_crit,
	 the magnitude is approximated by the max function, otherwise
	 the sqrt is returned. */
    
    ulong maxXY = max(labs(x),labs(y));
    if (maxXY > range) {
      return maxXY;
    } else {
      double sx = double(x)/sub_scale;
      double sy = double(y)/sub_scale;
      return (ulong) nlong(sqrt(sx*sx + sy*sy) * sub_scale);
    }
  }
  CoordinateClass& crossZ() {
    static CoordinateClass cc;
    cc.x = y;
    cc.y = -x;
    return cc;
  }
  //CoordinateClass Unit(long r =dist_crit) {
  //  return *this * sub_scale / Mag(r);
  //}
  DEF_OBJ_PUSHTO;
  DEF_OBJ_POPFROM;
  void print(ostream& =cout);
 /* private: */
  slong x;
  slong y;
};

extern char *logFilename;
extern char *clientLogFilename;

#ifdef SF_MAIN
/* global structures or objects */
CoordinateClass* Dir = 0;
CoordinateClass nullVector;
const char* funcName[17] =
{"power", "engine", "qdrive", "tube", "shield",
 "brake", "sensor", "fuel", "battery", "counter",
 "cloak", "boost", "dock", "factory", "beam", "ram",
 "armor"};
ofstream logFile;
//ofstream eventFile;
#else
extern CoordinateClass* Dir;
extern CoordinateClass nullVector;
extern const char* funcName[];
extern ofstream logFile;
//extern ofstream eventFile;
#endif

template<class T, class I> class Array {
public:
  Array(void) {
    p = NULL;
    n = I(0);
    err = &logFile;
  }
  Array(I i, ostream* o =&logFile) {
    p = NULL;
    n = I(0);
    initializeWith(i);
  }
  ~Array() {
    destroy();
  }
  int initializeWith(I i, ostream* o =&logFile) {
    destroy();
    p = new T[i];
    if (p)
      n = i;
    err = o;
    return !p;
  }
  void _initialize() {
    p = 0;
    n = 0;
    err = &logFile;
  }
  void initialize() {
    destroy();
    err = &logFile;
  }
  void destroy() {
    if (p) {

      if (!n)
	*err << "WARNING> attempting to delete a pointer allocated outside of class." << endl;

      delete[] p;
      p = 0;
      n = 0;

    }
  }
  Array<T,I>& operator=(Array<T,I>& a) {
    p = a.p;
    n = a.n;
    err = a.err;
    return *this;
  }
  //Array& operator=(T* t) {
  //  if (p)
  //    destroy();
  //  
  //  p = t;
  //  n = 0;
  //  err = &logFile;
  //  return *this;
  //}
  I extendBy(I i) {
    T* oldP = p;
    p = new T[n+i];
    
    if (!p) {
      *err << "ERROR> unable to expand array by " << i << endl;
      return 0;
    }

    short j;
    for (j=0; j<n; j++) {
      p[j] = oldP[j];
    }

    n += i;

    if ( oldP )
       delete[] oldP;

    return i;

  }
  I add(T& t) {
    if (extendBy(1)) {
      p[n-1] = t;
      return 1;
    } else
      return 0;
  }
  T& operator[](I i) const {
    /************************************************************/
    /* the return value may be used as an lvalue so it should   */
    /* be safeguarded in the event of a null pointer by return- */
    /* ing a static variable address.  In such an event error   */
    /* message will be the only indication that a problem oc-   */
    /* curred.                                                  */
    /************************************************************/

#ifdef DUMB_ARRAY
    return p[i];
#else
    static T a;

    if (p) {
      if (i >= 0 && i < n) {
	return p[i];
      } else {
	//if (err)
	//{
	//  *err << "ERROR> illegal attempt to access member " <<
	//    i << " out of " << n << endl;
	//}
	
	//
	// Uncomment these lines to generate an exception.
	//
	//char *ptr = (char *)0xCCCCCCCC;
	//*ptr = 0;

	return p[0];
      }
    } else {
      //if (err)
	//*err << "ERROR> illegal attempt to access member " <<
	//i << " from null array." << endl;
      return a;
    }
#endif
  }
  T* operator+(I i) {

#ifdef DUMB_ARRAY
    return p + i;
#else
    static T a;

    if (p) {
      if (i >= 0 && i < n) {
	return p + i;
      } else {
	//if (err)
	//  {
	//    *err << "ERROR> illegal attempt to access member " <<
	//      i << " out of " << n << endl;
	//  }
	
	//
	// Uncomment these lines to generate an exception.
	//
	//char *ptr = (char *)0xCCCCCCCC;
	//*ptr = 0;
	return p;
      }
    } else {
      //if (err)
      //*err << "ERROR> illegal attempt to access member " <<
      //  i << " from null array." << endl;
      return &a;
    }
#endif
  }
  int IsNotNull() {
    return p != 0;
  }
  T* rootOf() {
    return p;
  }
  I indexOf(T* t) {
    return t - p;
  }
private:
  T* p;
  I  n;
  ostream* err;
};

struct PopStruct {
  PopStruct() {
    billions = 0;
    lBillions = 0;
    checkSum = CheckSum();
  };

  ulong checkSum;
  DEF_ADD_TO_STAT(slong);
  DEF_SET_STAT(slong);
  DEF_ADD_TO_STAT(short);
  DEF_SET_STAT(short);
  ulong CheckSum()
    {
      return billions + lBillions;
    }
  void ClassCheck()
    {
      return;
    }

  void validate(short& b, slong& lb) {
    if (b > 0 && lb > 0 || b < 0 && lb < 0 || b == 0 || lb == 0) {
      /* check for lb overflow */
      if (lb < -long_billion) {
	SetStat(lb,lb % long_billion);
	AddToStat(b,-1);
	//lb = lb % long_billion;
	//b--;
      } else if (lb > long_billion) {
	SetStat(lb,lb % long_billion);
	AddToStat(b,1);
	//lb = lb % long_billion;
	//b++;
      }
    } else if (b > 0) { /* lb must be negative */
	AddToStat(lb,long_billion);
	AddToStat(b,-1);
	//lb += long_billion;
	//b--;
    } else { /* b must be negative */
      AddToStat(lb,-long_billion);
      AddToStat(b,1);
      //lb -= long_billion;
      //b++;
    }
  }
  slong log10() {
    if (billions > 0) {
      if (billions < 10)
	return 10;
      else
	return 9;
    } else {
      if (lBillions < 10)
	return 0; 
      else if (lBillions < 100)
	return 1; 
      else if (lBillions < 1000)
	return 2;
      else if (lBillions < 10000)
	return 3;
      else if (lBillions < 100000)
	return 4;
      else if (lBillions < 1000000)
	return 5;
      else if (lBillions < 10000000)
	return 6;
      else if (lBillions < 100000000)
	return 7;
      else
	return 8;
    }
    return 0;
  }
  slong InUnitsOf(slong unit) const {
    return billions * (long_billion / unit) + lBillions / unit;
  }
  double AsDouble() const {
    return billions * double(long_billion) + lBillions;
  }
  PopStruct(slong lb) {
    checkSum = CheckSum();
    SetStat(billions,0);
    SetStat(lBillions,lb);
    //billions  = 0;
    //lBillions = lb;
    validate(billions,lBillions);
  }
  PopStruct(short b, slong lb) {
    checkSum = CheckSum();
    SetStat(billions,b);
    SetStat(lBillions,lb);
    //billions  = b;
    //lBillions = lb;
    validate(billions,lBillions);
  }
  PopStruct(double p) {
    checkSum = CheckSum();
    SetStat(billions,short(p / long_billion));
    SetStat(lBillions,slong(p - billions*double(long_billion)));
    //billions = short(p / long_billion);
    //lBillions = long(p - billions*double(long_billion));
    validate(billions,lBillions);
  }
  PopStruct operator+(PopStruct p) {
    short b = (short) (p.billions + billions +
      (p.lBillions + lBillions) / long_billion);
    slong lb = (p.lBillions + lBillions) % long_billion;
    return PopStruct(b, lb);
  }
  short operator>(PopStruct p) {
    return billions > p.billions || billions == p.billions &&
      lBillions > p.lBillions;
  }
  short operator>=(PopStruct p) {
    return billions >= p.billions || billions == p.billions &&
      lBillions >= p.lBillions;
  }
  short operator<(PopStruct p) {
    return billions < p.billions || billions == p.billions &&
      lBillions < p.lBillions;
  }
  PopStruct operator-() {
    return PopStruct(-billions, -lBillions);
  }
  PopStruct operator-(PopStruct p) {
    return *this+(-p);
  }
  PopStruct operator+=(PopStruct p) {
    AddToStat(billions,p.billions);
    AddToStat(lBillions,p.lBillions);
    //billions += p.billions;
    //lBillions += p.lBillions;
    validate(billions, lBillions);
    return *this;
  }
  PopStruct operator-=(PopStruct p) {
    AddToStat(billions,-p.billions);
    AddToStat(lBillions,-p.lBillions);
    //billions -= p.billions;
    //lBillions -= p.lBillions;
    validate(billions, lBillions);
    return *this;
  }
  PopStruct operator*(slong f) const {
    slong lb = 0;

    if (f > 0 && lBillions >= 0 && lBillions < pc_long_max / f)
      lb = lBillions * f;
    else
      return PopStruct(AsDouble() * f);

    short b = (short) (billions * f);
    return PopStruct(b, lb);
  }
  PopStruct operator/(slong f) const {
    return PopStruct(AsDouble() / f);
  }
  /*  PopStruct operator*(double f) const { */
  /*      short b = (short) nlong(billions * f); */
  /*      long lb = nlong(lBillions * f); */
  /*      return PopStruct(b, lb); */
  /*    } */
  inline friend PopStruct Min(PopStruct a, PopStruct b) {
    if (b > a)
      return a;
    else
      return b;
  }
  slong Sqrt() {
    return 10*nlong(sqrt((double)max(InUnitsOf(100),0)));
  }
  void GrowBy(slong f, slong minDelta =0) {

    if (billions && billions * f / 100000 < 0) /* an overflow has occurred */
      return;

    slong d;

    if (billions > (short)(pc_long_max / 10000))
      /* an overflow is imminent -- cease growth */
      return;

    
    if (f > 100000) {

      if (lBillions > 10000000 || (f > 100100 && lBillions > 1000000))
	d = (lBillions / 100000 + billions * 10000) * (f-100000);
      else
	d = lBillions * (f-100000) / 100000 + 
	  billions * 10000 * (f-100000);

    } else {
      
      if (lBillions > 10000000 || (f < 99900 && lBillions > 1000000))
	d = (lBillions / 100000 + billions * 10000) * (f-100000);
      else
	d = lBillions * (f-100000) / 100000 + 
	  billions * 10000 * (f-100000);
    }
    
    AddToStat(lBillions,d + minDelta);
    //lBillions += d + minDelta;

    validate(billions, lBillions);
  }
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  char* print() {
    static char out[10];
    if (billions)
      sprintf(out, "%7.3f B", billions + lBillions/1e9);
    else if (lBillions >= 1000000) {
      float millions = (float)(lBillions/1000000.0);
      sprintf(out, "%7.3f M", millions);
    } else if (lBillions >= 1000) {
      float thousands = (float)(lBillions/1000.0);
      sprintf(out, "%7.3f K", thousands);
    } else
      sprintf(out, "%3ld", lBillions);
    return out;
  }
  DEF_OBJ_PUSHTO;
  DEF_OBJ_POPFROM;
  short billions;
  slong lBillions;
};

class ObjectBitmapClass {
  /* bitmap index and characteristics */
 public:
  friend class ModuleClass;
  friend class GenericClass;
  friend class PlanetClass;
  friend class DesignedClass;
  friend class ShipClass;
  friend class MissileClass;
  friend class GameStateClass;
  friend class GameBitmapClass;
  friend struct PingPacket;
  enum SequenceType            {spin, time, stationary, terminal,
				  transitional};
  short N()                      { return numFrames;}
  short Index()                  { return index; }
  short IsLoop()                 { return seq == spin || seq == time; }
  short IsSpin()           const { return seq == spin; }
  short IsTime()                 { return seq == time; }
  short IsTerminal()             { return seq == terminal; }
  short IsTransitional()         { return seq == transitional; }
  short Size()                   { return size; }
  CoordinateClass Dir(short i) const {
    return dir[(i+2*numFrames) % numFrames];
  }
  void Terminate() {
    index = -1;
  } 
  ObjectBitmapClass() {
    acOnly = FALSE;
    index = 0;
    numFrames = 0;
    size = 0;
    seq = 0;
    for ( int i=0; i<MAXANGLES; i++ )
      {
	dir[i].x = 0;
	dir[i].y = 0;
      }
  };
  ObjectBitmapClass(SequenceType, short, short, short);
  ~ObjectBitmapClass();
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  void init();
  void diff(char[], ObjectBitmapClass&);
  
  short              acOnly;
  short              index;
  short              numFrames;
  unsigned short     size;
  short              seq;
  short              alpha;
  short              gamma;
  CoordinateClass    dir[MAXANGLES];
};

class GameBitmapClass : public ObjectBitmapClass {
 public:
  friend class GameStateClass;
  GameBitmapClass() {}
  GameBitmapClass(SequenceType, short, short, short, char*, char*, char*);
  //short get(FILE*);
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  /*private:*/
  char Class[max_name_char];
  char raceName[max_name_char];
  char name[max_name_char];
  char file[max_name_char];
};


struct RankStruct {
  RankStruct() {
    duty = 0;
    dutyInterval = 0;
    accBonus = 0;
    totalScore = 0;
    activeScore = 0;
    totalKills = 0;
  }
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  char name[max_name_char];
  ulong  activeScore;
  ulong totalScore;
  ulong totalKills;
  short duty;
  short dutyInterval;
  short accBonus;
};

union ObjectUnion {
  class GenericClass *generic;
  class PlanetClass *planet;
  class DesignedClass *designed;
  class ShipClass *ship;
  class ColonyClass *colony;
  class MissileClass *missile;
  slong objLong;
};  

class RaceClass {
 public:
  friend class DesignedClass;
  friend class ShipClass;
  friend class GameStateClass;
  enum state {normal, victorious, defeated, hopeless};
  RaceClass() {
   id = 0; 
   totPop = 0;
   numRobots = 0;
   numPlanets = 0;
   maxPlanetRobots = 0;
   maxPopRobots = 0;
   distressObj.generic = 0;
   targetIndex = 0;
   targetIndexRank = 0;
   harvestIndex = 0;
   growthRate = 0;
   debrisOffset = 0;
   prefTemp = 0;
   prefAtmP = 0;
   prefGrav = 0;
   condition = 0;
   conditionType = 0;
   conditionSeconds = 0;
   conditionTiming = 0;
   conditionNumPar = 0;
   conditionTimePar = 0;
   numPlayerDeaths = 0;
   status = normal;
   bonus = 0;
   strategicPoints = 0;
   activeFrames = 0;
   fullColonyPct = 0;
  }
  void profile(class GameStateClass*, ostream* =0);
  char* ProfileCondition();
  char* GetPlanetList(short =0);
  short NumPlanetsInVictoryList(short* =0);
  short CalculateVictoryPct();
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
 /* private: */
  slong id;
  slong totPop;
  slong growthRate;
  slong conditionNumPar; /* new */
  ulong bonus;
  ulong strategicPoints;
  ulong activeFrames;
  ObjectUnion distressObj;
  short home;
  short targetIndex;
  short targetIndexRank;
  short harvestIndex;
  short numRobots;
  short numPlanets;
  short maxPlanetRobots;
  short maxPopRobots;
  short debrisOffset;
  short prefTemp;
  short prefAtmP;
  short prefGrav;
  short condition;
  short conditionType;
  short conditionTiming;
  short conditionSeconds;
  short conditionTimePar;
  short numPlayerDeaths;
  short status; /* new */
  short fullColonyPct;
  char name[max_name_char];
};

const char moduleKey[18] = "@@g @0@@@'c@y@  @";

class ModuleClass {
 public:
  enum ModuleFunctions {power_plant, engine, qdrv, tube, shield, brake,
			sensor, fuel_cell, battery, counter_measures,
			cloak, boost, dock, factory, beam, ram, armor,
			last_module};
  enum ModuleModes {standby, free, locked};
  enum TubeModes {auto_fire_mode = 1, manual_fire_mode = 2, not_upgradeable_mode = 4,
		  purchase_only_mode = 8};

  friend class DesignedClass;
  friend class ShipClass;
  friend class MissileClass;
  friend class GameStateClass;
  friend struct BusStruct;
  friend struct DesignStruct;
  void SetTubeMode(short m) {
    if ( check != 0xAAAAAAAA ) return;
    SetStat(par[4],par[4] | m);
    //par[4] = par[4] | m;
  }
  void ClearTubeMode(short m) {
    if ( check != 0xAAAAAAAA ) return;
    SetStat(par[4],par[4] ^ (par[4] & m));
    //par[4] = par[4] ^ (par[4] & m);
  }
  short CheckTubeMode(short m) {
    if ( check != 0xAAAAAAAA ) return 0;
    return (short)(par[4] & m);
  }
  void activate() { if ( check == 0xAAAAAAAA ) active = 1; }
  void deactivate() { if ( check == 0xAAAAAAAA ) active = 0; }
  ModuleClass() {
    initialize();
  }
  void initialize();
  void profile(class GameStateClass*);
  void Deplete(slong& dam) {
    if ( check != 0xAAAAAAAA ) return;
    /* apply weapon damage to shield */
    if (store > dam) {
      AddToStat(store,-dam * func / module_function_scale);
      MultToStat(dam,(module_function_scale - func) / module_function_scale);
      //store -= dam * func / module_function_scale;
      //dam *= (module_function_scale - func) / module_function_scale;
    } else {
      AddToStat(dam,-store * func / module_function_scale);
      MultToStat(store,(module_function_scale - func) / module_function_scale);
      //dam -= store * func / module_function_scale;
      //store *= (module_function_scale - func) / module_function_scale;
    }
  }
  void Replenish() {
    if ( check != 0xAAAAAAAA ) return;
    AddToStat(power,0);
    SetStat(store,-par[0] * power);
    //store = -par[0] * power;
  }
  void put(class Document*);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short get(ifstream&, SFString*, SFString*, short);
  slong value();
  short HasValidDepositFunction() {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(deposit == &ModuleClass::DepositStdPower ||
		 deposit == &ModuleClass::DepositCapPower ||
		 deposit == &ModuleClass::DepositFuel ||
		 deposit == &ModuleClass::DepositCharge ||
		 deposit == &ModuleClass::DepositXSPower);
  }
  short HasValidFireFunction() {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(fire && (fire == &ModuleClass::Thrust ||
			  fire == &ModuleClass::Warp ||
			  fire == &ModuleClass::Brake ||
			  fire == &ModuleClass::FireTube ||
			  fire == &ModuleClass::Scramble ||
			  fire == &ModuleClass::Hide ||
			  fire == &ModuleClass::Boost ||
			  fire == &ModuleClass::Dock ||
			  fire == &ModuleClass::Build));
  }
  short HasValidRequirementFunction() {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(requirement == &ModuleClass::StdPowerReq ||
		 requirement == &ModuleClass::CapPowerReq);
  }
  short HasValidWithdrawFunction() {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(withdraw == &ModuleClass::WithdrawStdPower ||
		 withdraw == &ModuleClass::WithdrawPower ||
		 withdraw == &ModuleClass::WithdrawFuel ||
		 withdraw == &ModuleClass::WithdrawCharge);
  }
  short HasValidStorageFunction() {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(storage == &ModuleClass::StdPowerStore ||
		 storage == &ModuleClass::CapPowerStore);
  }
  void IdNum(slong idNum) { if ( check == 0xAAAAAAAA ) id = idNum; }
  short IsActive() { return active; }
  short IsReady();
  short IsCharged() {
    if ( check != 0xAAAAAAAA ) return 0;
    return -power*par[0] == store;
  }
  void IncreaseValue(slong, slong);
  void AssignPortToShip(ShipClass*);
  void FireBeam(PingPacket*);
  void FireTube();
  void FireTubeWithIndex(short, GenericClass* =0);
  void FireTubeAtOffset(short, short, GenericClass* =0);
  void Brake();
  void Scramble();
  void Warp();
  void Thrust();
  void Hide();
  void Boost();
  void Dock();
  void Build();
  void Block();
  void ReceiveCompleteBuildMsg(class ShipClass*);
  void SendHideMsg();
  void ReceiveHideMsg(slong, slong);
  void SendUnHideMsg();
  void ReceiveUnHideMsg();
  void SendDockActivationMsg();
  void ReceiveDockActivationMsg(GenericClass*, slong, slong);
  void SendDockDeactivationMsg(char*);
  void ReceiveDockDeactivationMsg();
  void SendDockingSuccessfulMsg();
  void ReceiveDockingSuccessfulMsg(short/* , short, short*/);
  slong StdPowerStore();
  slong StdPowerReq();
  slong CapPowerStore();
  slong CapPowerReq();
  slong WithdrawFuel();
  slong WithdrawPower();
  slong WithdrawStdPower();
  slong WithdrawCharge();
  slong DepositFuel(slong&);
  slong DepositStdPower(slong&);
  slong DepositCapPower(slong&);
  slong DepositXSPower(slong&);
  slong DepositCharge(slong&);
  void Fire();
  slong XStorage();
  slong Storage();
  slong Requirement();
  slong Withdraw();
  slong Deposit(slong&);
  void Hit(slong& dam) { 
    if ( check != 0xAAAAAAAA ) return;
    slong newDamage = 
      (damage+dam < damageCap) ? ((damage+dam > 0)? damage+dam : 0) : 
      damageCap;
    AddToStat(dam,-(newDamage-damage));
    //dam -= newDamage - damage;
    SetStat(damage,newDamage);
    //damage = newDamage;
    SetStat(func,
	    (short)((damageCap-damage)*module_function_scale/max(damageCap,1)));
    //func = (damageCap-damage) * module_function_scale / max(damageCap,1);
  }
  void Lock() {
    if ( check != 0xAAAAAAAA ) return;
    if (mode == free)
      mode = standby;
    else if (mode == standby)
      mode = locked;
    else
      mode = free;
  }
  short GetMode() {
    return mode;
  }
  slong afMaxPar(slong f, short i) {
    if ( check != 0xAAAAAAAA ) return 0;
    if (power) 
      return (max(f,par[i]) * store)/(-power)*func/module_function_scale; 
    else return 0;
  }
  slong aPar(short i) { 
    if ( check != 0xAAAAAAAA ) return 0;
    if (power) 
      return (par[i] * store)/(-power)*func/module_function_scale; 
    else return 0;
  }
  slong cPar(short i, slong surplus =0) { 
    if ( check != 0xAAAAAAAA ) return 0;
    if (power) 
      return (par[i] * store + surplus)/(-power*par[0] + surplus)*func/module_function_scale; 
    else return 0;
  }
  slong tPar(short i, short j) {
    if ( check != 0xAAAAAAAA ) return 0;
    if (power) 
      return (min(par[i],par[j]) * store)/
	(-power*par[0])*func/module_function_scale; 
    else return 0;
  }
  slong fPar(short i) { return par[i] * func / module_function_scale; }
  slong Par(short i) {
    return par[i];
  }
  short IsEquivalentTo(ModuleClass* m) {
    if ( check != 0xAAAAAAAA ) return 0;
    return IsEquivalentTo(m->moduleFunc);
  }
  short IsEquivalentTo(short mf) {
    if ( check != 0xAAAAAAAA ) return 0;
    return short(moduleFunc == mf ||
		 moduleFunc == beam && (mf == tube || mf == ram) ||
		 moduleFunc == tube && (mf == beam || mf == ram) ||
		 moduleFunc == ram && (mf == beam || mf == tube));
  }
  float TubeRepeatRate(char frameRate) {
    if ( check != 0xAAAAAAAA ) return 0;
    if ( par[1] )
       return -power * frameRate / float(TubeEnergyCost());
    else
      return 0;
  }
  slong TubeBurst(slong reserve =0) {
    if ( check != 0xAAAAAAAA ) return 0;
    if ( par[1] && TubeEnergyCost() )
       return min((-power * par[0] + reserve) / TubeEnergyCost(), par[2]);
    return 0;
  }
  slong TubeEnergyCost();
  char* KeyOf(short& tubeNum) {
    static char blankKey[4] = "   ";
    static char key[4] = "[ ]";
    static char padKey[4] = "( )";
    if ( check != 0xAAAAAAAA ) return 0;
    if (moduleKey[moduleFunc] == '@') {
      return blankKey;
    } else if (!IsEquivalentTo(tube)) {
      if (moduleFunc != engine && moduleFunc != boost) {
	key[1] = moduleKey[moduleFunc];
	return key;
      } else {
	padKey[1] = moduleKey[moduleFunc];
	return padKey;
      }
    } else {

      tubeNum++;

      key[1] = '0' + tubeNum;

      return key;
    }
      
  }

  ulong checkSum;
  DEF_MULT_TO_STAT(slong);
  DEF_ADD_TO_STAT(slong);
  DEF_SET_STAT(slong);
  DEF_MULT_TO_STAT(short);
  DEF_ADD_TO_STAT(short);
  DEF_SET_STAT(short);

  ulong CheckSum()
    {
      ulong ret = store + damage + func + power + output + mass + damageCap + volume;
      for ( int i=0; i<nPar; i++ )
	{
	  ret += par[i];
	}
      return ret;
    }
  void ClassCheck()
    {
      return;
    }

  ulong check;

/* private:*/
  char name[max_module_name_char];
  slong id;
  slong raceID;
  slong power;
  slong mass;
  slong output;
  slong store;
  slong damageCap;
  slong damage;
  slong par[max_module_par];
  slong specPar[max_module_spar];
  slong workPar[max_module_wpar];
  ObjectUnion host;
  short func;
  short moduleFunc;
  short active;
  short frame;
  short volume;
  short debrisBitmapIndex;
  short activationSoundIndex;
  short deactivationSoundIndex;
  char nPar;
  char nSPar;
  char nWPar;
  char mode;
  void (ModuleClass::* fire) ();
  slong (ModuleClass::* storage) ();
  slong (ModuleClass::* requirement) ();
  slong (ModuleClass::* withdraw) ();
  slong (ModuleClass::* deposit) (slong&);
};

typedef void (ModuleClass::*ModuleFP)();

struct BusIdStruct {
  BusIdStruct() {
    short i, l;
    for (i = 0; i < max_module_list; i++) {
      moduleIdList[i] = 0;
      for (l = 0; l < max_module_spar; l++)
	moduleParList[i*max_module_spar+l] = 0;
      }
  }
  BusIdStruct(slong n) {
    numSlots = n;
    numMod = 0;
    short i, l;
    for (i = 0; i < max_module_list; i++) {
      moduleIdList[i] = 0;
      for (l = 0; l < max_module_spar; l++)
	moduleParList[i*max_module_spar+l] = 0;
      }
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short CompareTo(BusIdStruct& b) {
    short s = numSlots == b.numSlots && numMod == b.numMod;
    short i,j;
    for (i=0; i<numSlots; i++) {
      s = s && moduleIdList[i] == b.moduleIdList[i];
      for (j=0; j<max_module_spar; j++) {
	s = s &&
	  moduleParList[i*max_module_spar + j] ==
	  moduleParList[i*max_module_spar + j];
	if (s == 0)
	  return 0;
      }
      if (s == 0)
	return 0;
    }
    return s;
  }
  /* data */
  slong numSlots;
  slong numMod;
  slong moduleIdList[max_module_list];
  slong moduleParList[max_module_list*max_module_spar];
};

struct BusStruct {
  enum BusMessage {no_message, fire_brake, release_brake, fire_thrust_forward,
		 fire_thrust_reverse, fire_thrust_left, fire_thrust_right,
		 release_thrust, fire_tubes, fire_warp, release_warp,
		 fire_turn_left, fire_turn_right, release_turn, scramble,
		 fire_cloak, release_cloak, fire_boost, release_boost, 
		 fire_dock, release_dock, fire_tubes_at_angle};
  BusStruct() {
    InitializeWith(0);
  }
  BusStruct(short n) {
    InitializeWith(n);
  }
  ~BusStruct() {
    }
  void InitializeWith(short n) {
    check = 0xAAAAAAAA;
    modPtr = 0;
    msg = no_message;
    msgPriority = 0;
    numSlots = n;
    numMod = 0;
    numPorts = 0;
    numTubes = 0;
    numBeams = 0;
    numPowerPlants = 0;
    powerOut = 0;
    powerIn = 0;
    powerRate = 0;
    aveFunc = 0;

    if (n)
      moduleList.initializeWith(n);
    else
      moduleList.initialize();

    ClearSingleModuleMaps();

  }
  void profile(GameStateClass* g) {
    short i;
    for (i = 0; i < numMod; i++)
      moduleList[i].profile(g);
  }
  void FreeModuleList();
  void ClearSingleModuleMaps() {
    /* single module pointers */
    warpMod = 0;
    impulseMod = 0;
    brakeMod = 0;
    shieldMod = 0;
    scrambleMod = 0;
    stealthMod = 0;
    boostMod = 0;
    sensorMod = 0;
    factoryMod = 0;
    armorMod = 0;
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  void MapFunctions(class DesignedClass*, class DesignedClass*);
  void RemapFunctions(class DesignedClass*, class DesignedClass*, 
		      char* = "no reason");
  void MapPorts();
  void ProcessMessage(DesignedClass*);
  void RepairSystems(short, short =0);
  void BudgetPower();
  void FireModules(DesignedClass*);
  inline void FireWeaponModules(ShipClass*, CoordinateClass&,
			 CoordinateClass&,
			 short);
  inline void FireDefensiveWeaponModules(ShipClass*, CoordinateClass&,
			 CoordinateClass&,
			 short);
  void DecideWhichWeaponsToFire(ShipClass*,CoordinateClass& =nullVector,
				CoordinateClass& =nullVector, short=0);
  void FireSelectedWeapons(ShipClass* s);
  void DecideWhichDefensiveWeaponsToFire(ShipClass*,CoordinateClass& =nullVector,
				CoordinateClass& =nullVector, short=0);
  void FireSelectedDefensiveWeapons(ShipClass* s);
  void WithdrawSurplusPower(slong);
  slong CalculateSurplusPower();
  slong CalculatePowerCapacity();
  void ReloadTubes();
  void ReplenishSystems();
  short IsDockFree(short i) {
    return dockMod && i<numPorts && dockMod[i] && !dockMod[i]->workPar[1];
  }
  short IsAnyDockFree() {
    if (!dockMod)
      return -1;
    short i;
    for (i = 0; i < numPorts; i++)
      if (dockMod[i] && !dockMod[i]->workPar[1])
	return i;

    return -1;
  }
  /* data */
  ulong check;
  uchar  modPtr;
  short msg;
  short msgPriority;
  short numSlots;
  short numMod;
  short numPorts;
  short numPowerPlants;
  short numTubes;
  short numBeams;
  slong powerOut;
  slong powerIn;
  slong powerRate;
  slong aveFunc;
  ModuleClass* warpMod;   /* mapped to module with warp function */
  ModuleClass* impulseMod;   /* mapped to module with impulse function */
  ModuleClass* brakeMod; /* mapped to module with brake function */
  ModuleClass* shieldMod; /* mapped to module with shield function */
  ModuleClass* scrambleMod; /* mapped to module with scramble function */
  ModuleClass* stealthMod; /* mapped to module with cloak function */
  ModuleClass* boostMod; /* mapped to module with boost function */
  ModuleClass* sensorMod; /* mapped to module with sensor function */
  ModuleClass* factoryMod; /* mapped to module with build function */
  ModuleClass* armorMod; /* mapped to the module with the armor function */
  ModuleClass* dockMod[max_module_list]; /* module list with dock function */
  ModuleClass* tubeMod[max_module_list]; /* module list with tube function */
  ModuleClass* beamMod[max_module_list]; /* module list with beam function */
  ModuleClass* powerMod[max_module_list]; /* module list with power function */
  Array<ModuleClass, short> moduleList; //[max_module_list];
};

struct PingPacket {
  /* holds distance, displacement vector, and pointer to target object */
  ulong   dist;
  slong            frame;
  CoordinateClass disp;
  CoordinateClass vel;
  ObjectUnion     obj;
  PingPacket() {
    _initialize();
  }
  inline PingPacket(slong, CoordinateClass, CoordinateClass, GenericClass*);
  inline PingPacket(GenericClass* self, GenericClass* target,
	     slong =dist_crit, 
	     CoordinateClass& =nullVector);
  inline PingPacket Sniff(GenericClass*, GenericClass*,
			  slong = pc_long_max);
  inline PingPacket& Sniff(PingPacket&, slong);
  void _initialize();
  void initialize() {
    disp = nullVector;
    vel = nullVector;
    _initialize();
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  PingPacket Invert(GenericClass* g) {
    return PingPacket(dist, disp*-1, vel*-1, g);
  }
};

class PlayerClass {
  /* object for holding player information */
 public:
  friend class ShipClass;
  friend class GameStateClass;
  enum PlayerModes {active=1, locked=2, developer_mode=4, pending=8,
		    veteran_mode=16, probation=32, demo_mode=64};
  PlayerClass(char*, char*, char*, char*,slong,slong, slong, slong, ulong, slong, slong, char, char =FALSE,slong=0);
  PlayerClass();
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  void profile(GameStateClass* g);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  void activate() {
    mode = mode | (active + locked);
  }
  void deactivate() {
    mode = mode ^ (mode & active);
  }
  void EnterGame() {
    mode = mode | veteran_mode;
  }
  void SetNewPlayerMode() {
    mode = mode ^ (mode & veteran_mode);
  }
  void LockSlot() {
    mode = mode | locked;
  }
  void FreeSlot() {
    mode = mode ^ (mode & (locked + active));
  }
  void SetDeveloperMode() {
    mode = mode | developer_mode;
  }
  void SetPlayerMode() {
    mode = mode ^ (mode & (locked + developer_mode));
  }
  short IsDeveloper() {
    return mode & developer_mode;
  }
  void PutOnProbation() {
    mode = mode | probation;
  }
  short IsDemoPlayer() {
    return mode & demo_mode;
  }
  void TakeOffProbation() {
    mode = mode ^ (mode & probation);
  }
  short IsOnProbation() {
    return mode & probation;
  }
  short IsActive() {
    return mode & active;
  }
  short SlotIsAvailable() {
    return (mode & locked) == 0;
  }
  short IsVeteran() {
    return mode & veteran_mode;
  }
  void SetPendingFlag() {
    mode = mode | pending;
  }
  void ClearPendingFlag() {
    mode = mode ^ (mode & pending);
  }
  short IsPending() {
    return mode & pending;
  }
  short IsRestrictedFromUsing(ModuleClass* m)
    {
      
      if ( !m )
	{
	  return 0;
	}
      
      if ( m->check != 0xAAAAAAAA )
	{
	  return 0;
	}
      
      switch ( m->moduleFunc )
	{  
	case ModuleClass::beam:
	case ModuleClass::ram:
	  return IsDemoPlayer();
	default:
	  return 0;
	}
      return 0;
    }
  
  void ReceiveRankingMsg(short, short);
  int CalculateRank(short,short =0);
  short CrewExperienceLevel() {
    return (short)(activeScore / score_to_crew_experience_level);
  }
  ulong CheckSum() {
    return activeScore + totalScore + strategicPoints +
      numKills + numKillsTotal +
      numKilled + numKilledThisSession +
      refValue + rank;
  }
  void ClassCheck() {
    if ( refValue > max_credit )
      refValue = max_credit;
  }
 /* private: */
  char name[max_name_char];
  char login[max_name_char];
  char shipName[max_name_char];
  char ipAddress[max_name_char];
  slong serial;
  char mode;
  slong raceId;
  slong shipId;
  slong revertRaceId;
  ulong activeFrames;
  ulong secondsOnline;
  ulong activeScore;
  ulong totalScore;
  ulong strategicPoints;
  slong refValue;
  short shipIndex;
  short rank;
  ulong numMissions;
  ulong numKills;
  ulong numKillsTotal;
  ulong numKilled;
  ulong numKilledThisSession;

  ulong checkSum;
  DEF_ADD_TO_STAT(ulong);
  DEF_SET_STAT(ulong);
  DEF_ADD_TO_STAT(slong);
  DEF_SET_STAT(slong);
  DEF_ADD_TO_STAT(short);
  DEF_SET_STAT(short);
  //void AddToULongStat(unsigned long *,long);
  //void SetULongStat(unsigned long *,unsigned long);
  //void AddToLongStat(long *,long);
  //void SetLongStat(long *,long);
};

class GenericClass {
  /* generic object -- planet, ship (derived), missile (derived) */
 public:
 /* private: */
#ifdef UNIX
  /************ Leading Virtual Function Table Pointer ****************/
  /* SGI uses a trailing vft, so leave room for network compatability */
  /********************************************************************/
  void* vft;
#endif
  /* longs and pointers */
  slong check;
  slong frame;
  slong bitmapStartFrame;
  slong nextGravityFrame;
  slong mass;
  slong gmass;
  ObjectUnion parent;
  slong raceID;
  /* Compound Objects */
  CoordinateClass now;
  CoordinateClass vel;
  CoordinateClass tvel;
  CoordinateClass gnow;
  CoordinateClass gvel;
  ObjectBitmapClass bitmap;               // pointer to bitmap (index)
  ObjectBitmapClass lastBitmap;           // pointer to bitmap (last index)
  /* shorts */
  short gframe;
  short gfresh;
  short cycle;
  short originalBitmapIndex;
  short numPings;
  short typeOfObject;
  /* chars */
  char offset;
  char active;
/* public: */
  friend class GameStateClass;
  friend struct PingPacket;
  friend class ModuleClass;
  friend class DesignedClass;
  friend class MissileClass;
  friend class ShipClass;
  GenericClass() {
    initialize();
  }
  virtual ~GenericClass() {}
  GenericClass(const GenericClass& g, 
	       ObjectBitmapClass& objectBitmap, slong v) {
    initializeWith(g, objectBitmap, v);
  }
  GenericClass(short i, ObjectBitmapClass& objectBitmap) {
    initializeWith(i, objectBitmap);
  }
  void initialize();
  void initializeWith(short i, ObjectBitmapClass& objectBitmap);
  void initializeWith(const GenericClass&, ObjectBitmapClass&, slong);
  void diff(GenericClass*);
  void Recycle();
  virtual void Update() {}
  virtual char* Query(ShipClass* g, short post =1, short force =0) {
    return 0;
  }
  virtual void Explode(char* ="no reason", short =1, short =0) {}
  virtual void Acquire(ulong =pc_ulong_max) {}
  virtual void ProfileCheckSum(class GameStateClass*);
  virtual slong CheckSum(class GameStateClass*);
  virtual void ShortProfile();
  virtual void ReceiveObjectTrackingMsg(class SF_Msg*, short =1);
  virtual void SendObjectTrackingMsg();
  virtual slong Mass(slong delta =0, short include =1) const {
    return mass;
  }
  slong* AddressOfVirtualFunctionTable() {
    #if defined(WIN32) || defined(OS2)
    return (slong*)this;
    #endif
    #if defined(UNIX)
    return 0L;
    #endif
  }
  short IsFriendlyTo(GenericClass* d){
    if ( IsValidObjectPointer(d) )
      {
	return raceID == d->raceID;
      }
    return 0;
  }
  short IsEnemyOf(GenericClass* d){
    if ( IsValidObjectPointer(d) )
      {
	return raceID != d->raceID;
      }
    return 0;
  }
  virtual short CanBeSeenBy(class DesignedClass* d)
    { return 1; } /* Planets can always be seen */
  virtual short IsOutOfRangeOf(class DesignedClass* d, slong) {
    return 0;
  }
  CoordinateClass& operator-(const GenericClass& g) const {
    static CoordinateClass c;
    c = now - g.now;
    return c;
  }
  CoordinateClass& Attract(PingPacket&, short =1, short =0);
  CoordinateClass& VelKep(PingPacket&);
  virtual PingPacket& Ping(TargetType =no_target,
			   TargetSelector =nearest,
			   short =0,
			   ulong =pc_long_max,
			   CoordinateClass =nullVector);
  virtual slong cX(slong scale =sub_scale, slong nFrames =0) { 
    return (now.x+tvel.x*nFrames)/scale;
  }
  virtual slong cY(slong scale =sub_scale, slong nFrames =0) { 
    return (now.y+tvel.y*nFrames)/scale;
  }
  void PlaySound(short soundIndex);
  void PlaySoundOnScreen(short soundIndex, short localOnly =0);
  void activate() {
    active = 1;
  }
  void deactivate() {
    active = 0;
  }
  short IsActive() {
    return active && !(lastBitmap.index == -1 && bitmap.index == -1);
  }
  short IsDone()
    { return bitmap.index == -1; }
  short IsLeaving() { return lastBitmap.index == -1; }
  virtual short IsExploding() { return 0; }
  virtual short IsDesigned() { return 0; }
  short Index() { return bitmap.index; }
  short LastIndex() { return lastBitmap.index; }
  slong X(slong scale =sub_scale, slong nFrames =0) {
    return (now.x+tvel.x*nFrames)/scale;
  }
  slong Y(slong scale =sub_scale, slong nFrames =0) {
    return (now.y+tvel.y*nFrames)/scale;
  }
  char Offset() { return offset; }
  void ApplyThrust(CoordinateClass th, slong speedLimit) {
    vel.AddSubjectToLimit(th / max(mass,1) * dt, speedLimit);
  }
  ObjectType Type() const { return ObjectType(typeOfObject); }
  virtual slong Hit(slong, DesignedClass*, CoordinateClass&,
		   short =100, short =0, short =0) {
    return 0;
  }
  virtual TargetType TType() const { return no_target; }
  virtual char* TypeName() const { return "generic"; }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  virtual void profile(class GameStateClass*);
  char vis;
  slong currentSound;
#if defined(OS2) || defined(WIN32)
  /************ Trailing Virtual Function Table Pointer ***************/
  /* OS2 uses a leading vft, so leave room for network compatability  */
  /********************************************************************/
  void* vft;
#endif
};


class PlanetClass : public GenericClass{
 public:
  PlanetClass();
  virtual ~PlanetClass() {}
  TargetType TType() const {
    return planet_target;
  }
  char* TypeName() const { return "planet"; }
  short CanBeSeenBy(class DesignedClass* d)
    { return 1; } /* Planets can always be seen */
  short get(ifstream&, SFString*, SFString*, short); 
  void put(class Document*);
  void Explode(char* ="no reason", short =1, short =0);
  void Update();
  char* Query(ShipClass* g, short post =1, short force =0) { return 0; }
  void profile(class GameStateClass*);
  void ProfileCheckSum(class GameStateClass*);
  slong CheckSum(class GameStateClass*);
  slong Hit(slong, DesignedClass*, CoordinateClass&,
	   short =100, short =0, short =0);
  slong ExploitationPotential();
  slong ExploitationLevel();
  short Habitability(RaceClass&, short =1);
  PopStruct OptimalPopulation(class RaceClass&);
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  /* data */
  char name[max_name_char];
  ColonyClass* colony;
  ulong orbitalRadius;
  slong iPhase;
  slong  terraForm;
  short revertRaceIndex;
  short revertColonyDesignIndex;
  short temp;
  short atmP; /* new */
  short grav; /* new */
  short accBonus;
  short minor;
  char hydpc;
  char enemyOrbiting;
};

class DesignedClass : public GenericClass {
  /* man-made object -- ship (derived), missile (derived) */
  /* private:*/
 public:
  /* compound objects */
  BusStruct bus;
  CoordinateClass comVec;
  CoordinateClass comVel;
  PingPacket targetPP;
  /* longs and pointers */
  slong assignPar1;
  slong assignPar2;
  slong dutyPar;
  slong maxSpeed;
  slong sensorThreshold;
  slong numLagFrames;
  ObjectUnion viewObj;
  ObjectUnion lockObj;
  ObjectUnion lastAttacker;
  /* shorts */
  short stealth;
  short repairRate;
  short assign;
  short assignPriority;
  short assignOverRide;
  short stat;
  short duty;
  short thrustOffset;
  short firingTubes;
  /* chars */
  RotationType heading;
  RotationType seekHeading;
  signed char turning;
  signed char seekTurning;
  char throttle;
  char transitSlide;
  char mode;
  char rotRate;
/* public: */
  enum assignment {manual, patrol, go_planet, go_nearest_planet,
		   go_home,
		   establish_orbit, attack, seek_heading, dry_dock,
		   follow_object, go_dock, go_multi_dock, self_destruct,
		   last_assignment
  };
  enum mode { mode_auto = 1, mode_shift = 2, mode_control = 4, mode_orbit = 8, mode_left=16, mode_right=32 };
  enum status {idle, orbiting, leaving_orbit, warping, entering_warp,
	       exiting_warp, docked, under_construction,
	       awaiting_construction, standby, last_status};
  enum HitType {heat_hit = 1, bomb_hit = 2, beam_hit=4, bypass_push_hit=8, bypass_armor_hit=16};
  friend class GameStateClass;
  friend struct PingPacket;
  friend class ModuleClass;
  friend class MissileClass;
  friend class ShipClass;
  virtual ~DesignedClass() {}
  DesignedClass() {
    initialize();
  }
  DesignedClass(const DesignedClass& g, 
		ObjectBitmapClass& objectBitmap, slong v,
		slong* par) {
    initializeWith(g, objectBitmap, v, par);
  }
  DesignedClass(short i, ObjectBitmapClass& objectBitmap) {
    initializeWith(i, objectBitmap);
  }
  void initialize();
  void initializeWith(const DesignedClass&, ObjectBitmapClass&, slong, slong* =0);
  void initializeWith(short, ObjectBitmapClass&);
  void ReceiveTerminateWarpMsg(CoordinateClass& v =nullVector); 
  inline void SendTerminateWarpMsg(CoordinateClass& v =nullVector); 
  inline void CheckTransition(CoordinateClass& v =nullVector,
			      short =0); 
  void ReceiveInitiateWarpMsg();
  void SendInitiateWarpMsg();
  void WarpTransition();
  virtual void AssignDesign(short) {
  }
  virtual void AssignLocation(short) {
  }
  slong HasValidModulePointers(short& i) {
    for (i=0; i<bus.numMod; i++) {
      if (!bus.moduleList[i].HasValidDepositFunction()) {
	return module_deposit_corrupt;
      }
      if (bus.moduleList[i].fire && !bus.moduleList[i].HasValidFireFunction()) {
	return module_fire_corrupt;
      }
      if (!bus.moduleList[i].HasValidWithdrawFunction()) {
	return module_withdraw_corrupt;
      }
      if (!bus.moduleList[i].HasValidRequirementFunction()) {
	return module_requirement_corrupt;
      }
      if (!bus.moduleList[i].HasValidStorageFunction()) {
	return module_storage_corrupt;
      }
    }
    return no_error;
  }
  slong IsValidDesignedObjectPointer();
  slong IsValidDesignedObject(short, char*);
  slong Mass(slong delta =0, short include =1) const {

    slong tmass = 0;

    if (bus.numSlots) {
      tmass = bus.numSlots * hull_mass_per_slot + delta;
      
      short i;
      for (i=0; i<bus.numMod; i++)
	tmass += bus.moduleList[i].mass;
    } else /* this shouldn't happen */
      tmass = 1000;

    return tmass;
  }
  void CalculateMass(slong delta =0, short include =1) {
    mass = Mass(delta, include);
    gmass = mass;
  }
  void Command(short m, short priority =0) {
    if (!bus.msgPriority) {
      bus.msg = m;
      bus.msgPriority = priority;
      bus.ProcessMessage(this);
    }
  }
  void Command(short m , CoordinateClass arg1, CoordinateClass arg2, 
	       short priority =0) {
    if (!bus.msgPriority) {
      bus.msg = m;
      comVec = arg1;
      comVel = arg2;
      bus.msgPriority = priority;
      bus.ProcessMessage(this);
    }  
  }
  void Update();
  char* Query(ShipClass* s, short post =1, short force =0) { return 0; }
  slong TotalValue() {
    short i;
    slong total = 0;
    for (i=0; i<bus.numMod; i++)
      total += bus.moduleList[i].value();
      
    return total;
  }
  slong EffectiveValue() {
    return TotalValue() / module_function_scale * bus.aveFunc;
  }
  slong Hit(slong, DesignedClass*, CoordinateClass&,
	   short =100, short =0, short =0);
  short GetMode(short m) {
    return mode & m;
  }
  short AutoResponse() {
    return GetMode(mode_auto);
  }
  short HasShift() {
    return GetMode(mode_shift);
  }
  short HasControl() {
    return GetMode(mode_control);
  }
  short AutoOrbit() {
    return GetMode(mode_orbit);
  }
  short HasLeftButtonDown() {
    return GetMode(mode_left);
  }
  short HasRightButtonDown() {
    return GetMode(mode_right);
  }
  short SeekHeading(const CoordinateClass&);
  short SeekHeading(const slong);
  short CanBeSeenBy(DesignedClass* d) { return vis; }
  short IsInFrontOf(DesignedClass* d) {
    int i = 0;
    if ( IsValidObjectPointer(d) )
      {
	i = d->heading;
	if ( i < 0 ) 
	  return 0;
	return (d->bitmap.Dir(d->heading) * (now-d->now)) > 0;
      }
    return 0;
  }
  short IsLeftOf(DesignedClass* d) { 
    int i;
    if ( IsValidObjectPointer(d) )
      {
	i = (d->heading + d->bitmap.numFrames/4) % d->bitmap.numFrames;
	if ( i < 0 )
	  return 0;
	return (d->bitmap.Dir(i) * (now-d->now)) > 0;
      }
    return 0;
  }
  /*short IsInFiringArcOf(DesignedClbass* d, long offset)  {
    CoordinateClass norm = (now-d->now);
    norm = norm * sub_scale / (norm.Mag()+1);
    return (d->bitmap.dir[(d->heading+offset+bitmap.numFrames) %
			  d->bitmap.numFrames] * norm) /
	    sub_scale > 9*sub_scale/10;
  }*/
  /*CoordinateClass VSeek(PingPacket& pp, long sens = 10, long lead =0) {
    return SeekThrust(pp, sens, lead, 
		      bus.impulseMod->aPar(0)*dt*sub_scale/mass);
  }*/
  short IsMobile() {
    return bus.impulseMod != 0;   /* port: was "> 0" (pointer vs 0) */
  }
  short IsWarping(short includeTransition =3) {
    return bus.warpMod && (stat == DesignedClass::warping ||
			   (stat == entering_warp && (includeTransition & 1) ||
			    stat == exiting_warp && (includeTransition & 2)));
  }
  short IsTransitioning() {
    return bus.warpMod && (stat == entering_warp || stat == exiting_warp || bitmap.IsTransitional());
  }
  slong HasDetectionLevelOf(GenericClass* g, slong lightSpeed) {
    slong val = 0;
    if ( IsValidObjectPointer(g) )
      {
	if (bus.sensorMod) 
	  val = min_sensor_value + bus.sensorMod->fPar(0);
	else
	  val = min_sensor_value;
	
	val = nlong(val * g->mass * (1 + double(g->tvel.Mag()) /
				     lightSpeed) /
		    ((*this-*g).Mag() / sensor_dist + 1));
      }
    return val;
  }
  short CanSee(DesignedClass* g, slong lightSpeed) {
    if ( IsValidObjectPointer(g) )
      {
	return 
	  g->vis && 
	  HasDetectionLevelOf(g, lightSpeed) > g->sensorThreshold *
	  g->stealth;
      }
    return 0;
  }
  short IsOutOfRangeOf(DesignedClass* g, slong lightSpeed) {
    if ( IsValidObjectPointer(g) )
      {
	return g->HasDetectionLevelOf(this, lightSpeed) < sensorThreshold;
      }
    return 0;
  }
  short Volume() {
    short volume = 0;
    short i;
    for (i=0; i<bus.numMod; i++)
      volume += bus.moduleList[i].volume;

    return volume;
  }
  unsigned short SeekThrust(PingPacket&, slong, slong, slong, slong, short =1);
  unsigned short FleeThrust(PingPacket&, slong, slong, slong);
  ulong SeekCore(CoordinateClass&, PingPacket&, slong, slong, slong, slong);
  ulong FleeCore(CoordinateClass&, PingPacket&, slong, slong, slong);
  /* CoordinateClass VFlee(PingPacket&, long = 5,  long = 5); */
  short ModuleFunc(short j) { 
    if (bus.moduleList.IsNotNull())
      return bus.moduleList[j].func;
    else
      return 0;
  }
  short ModuleStore(short);
  short ModuleType(short j) {
    return bus.moduleList[j].moduleFunc;
  }
  short NumModules() { return bus.numMod; }
  void SeekObj(PingPacket& pp, slong sens =10, slong vSens =5) {
    if ( IsValidObjectPointer(pp.obj.generic) )
      {
	SeekObj(pp, sens, vSens, pp.obj.generic->bitmap.size/2);
      }
  }
  void SeekObj(PingPacket&, slong, slong, slong);
  void FleeObj(PingPacket&, slong =5, slong =5);
  void Repel(PingPacket&);
  void Avoid(PingPacket& pp) {
    if (pp.dist < (int)avoid_dist)
      AvoidCore(pp);
  }
  void AvoidCore(PingPacket&);
  void Acquire(ulong =pc_long_max) {}
  void Coast() ;
  void AccForward() { thrustOffset = 0;}
  void AccReverse() { thrustOffset = bitmap.numFrames/2;}
  void AccLeft() { thrustOffset = bitmap.numFrames/4;}
  void AccRight() { thrustOffset = 3*bitmap.numFrames/4;}

  void TurnLeft();
  void TurnRight();
  void Straight();
  void SetMode(short, short);
  void View(GenericClass* g) {
    viewObj.generic = g;
  }
  void Expode() {}
  void ClearAIParams() {
    ClearAssign();
    seekTurning = 0;
    LeaveOrbit();
  }
  void ClearAssign() {
    assign = manual;
    assignPriority = 0;
  }
  void SwapAssign(slong over= manual) {
    assign = assignOverRide;
    assignOverRide = (short)over;
    assignPriority = 0;
  }
  void BreakOrbitalLock() {
    seekTurning = 0;
  }
  void EstablishOrbit(slong over= manual) {
    if (bus.impulseMod && stat != under_construction &&
	stat != awaiting_construction) {
      assign = establish_orbit;
      assignOverRide = (short)over;
      assignPriority = 0;
      if (!IsWarping())
	stat = idle;
    } else
      assign = (short)over;
  }
  void LeaveOrbit() {
    if (stat == orbiting && bus.impulseMod) {
      stat = leaving_orbit;
      cycle = 1;
      seekTurning = 0;
      SetMode(mode_orbit, op_off);
    }
  }
  void GoPlanet(slong p, slong over= manual) {
    assign = go_planet;
    assignPar2 = p;
    assignOverRide = (short)over;
    assignPriority = 0;
    LeaveOrbit();
  }
  void GoPlanet() {
    assign = go_planet;
    assignOverRide = manual;
    assignPriority = 0;
    LeaveOrbit();
  }
  void GoHome(slong over =manual) {
    assign = go_home;
    assignPriority = 0;
    assignOverRide = (short)over;
  }
  void GoNearestPlanet(GenericClass* p, slong over= manual) {
    PingPacket nearPP = Ping(planet_target, nearest);
    if ( IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) ) 
      GoPlanet(nearPP.obj.generic - p, over);
    else
      ClearAssign();
  }
  void GoDock(slong over =manual) {
    assign = go_multi_dock;
    assignOverRide = (short)over;
    assignPriority = 0;
    LeaveOrbit();
  }
  void FollowTarget() {
    assign = follow_object;
    assignPriority = 0;
  }
  void AttackTarget() {
    if (IsValidObjectPointer(targetPP.obj.generic) && 
	targetPP.obj.generic->IsFriendlyTo(this)) 
      FollowTarget();
    else {
      assign = attack;
      assignPriority = 0;
    }
    seekTurning = 0;
    if (bus.impulseMod)
      LeaveOrbit();
  }
  short FollowObj(GenericClass* t, short priority =0) {
    if (Assign(follow_object, slong(t), priority)) {
      if (bus.impulseMod)
	LeaveOrbit();
      return 1;
    }
    return 0;
  }
  void AttackObj(GenericClass* t, short priority =0) {
    if ( IsValidObjectPointer(t) )
      {
	if (!t->IsFriendlyTo(this)) {
	  Assign(attack, slong(t), priority);
	} else {
	  Assign(follow_object, slong(t), priority);
	}
	seekTurning = 0;
	if (bus.impulseMod)
	  LeaveOrbit();
      }
  }
  short IsExploding() {
    return 0;
  }
  short IsDesigned() {
    return 1;
  }
  slong cX(slong scale =sub_scale, slong nFrames =0) { 
    if ( IsValidObjectPointer(viewObj.generic) &&
	 viewObj.generic != this && 
	 viewObj.generic->IsActive() )
      return (viewObj.generic->now.x+
	      viewObj.generic->tvel.x*nFrames)/scale;
    else if ( IsValidObjectPointer(parent.generic) )
      return ((parent.generic->now.x+parent.generic->tvel.x*nFrames)/
	      scale*(transitSlide+nFrames) +
	      (now.x+tvel.x*nFrames)/scale*(max_slide-transitSlide-nFrames)) /
	max_slide;
    else
      return (now.x+tvel.x*nFrames)/scale;
  }
  slong cY(slong scale =sub_scale, slong nFrames =0) { 
    if ( IsValidObjectPointer(viewObj.generic) &&
	 viewObj.generic != this && 
	 viewObj.generic->IsActive() )
      return (viewObj.generic->now.y+viewObj.generic->tvel.y*nFrames)/scale;
    else if ( IsValidObjectPointer(parent.generic) )
      return ((parent.generic->now.y+parent.generic->tvel.y*nFrames)/
	      scale*(transitSlide+nFrames) +
	      (now.y+tvel.y*nFrames)/scale*
	      (max_slide-transitSlide-nFrames)) / max_slide;
    else
      return (now.y+tvel.y*nFrames)/scale;
  }
  /*PingPacket Ping(TargetType =no_target,
    TargetSelector =nearest, short =0,
    long =pc_long_max,
    CoordinateClass =nullVector) {
    return PingPacket();
    }*/
  short SpawnBeam(ModuleClass*, PingPacket* =0);
  short Spawn(struct MissileDesignStruct&, slong*, slong =0, slong =-1, GenericClass* =0);
  void SpawnRandomModules(short, CoordinateClass* =0);
  void Displace(const DesignedClass&, slong, slong*);
  short Assign(assignment a, slong par =0, short aP =0) {
    if ((aP >= assignPriority || assignPriority == 0) &&
	stat != under_construction && stat != awaiting_construction) {
      assign = a;
      assignPriority = aP;

      /* colonies should not forget about their parent */
      if (Type() == ship)
	parent.generic = 0; 

      if (stat != warping && stat != exiting_warp && stat != entering_warp)
	stat = idle;

      seekTurning = 0;
      if (assign == attack || assign == follow_object)
	  targetPP.obj.objLong = par;
      else if (assign == seek_heading)
	seekHeading = (char)par;
      viewObj.generic = this;
      return 1;
    }
    return 0;
  }
  short Assign() {
    return short(assign);
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  void Brake(DesignedClass& obj) {
    obj.vel = (obj.vel*obj.bus.brakeMod->par[0])/100;
  }
  void profile(GameStateClass*);
  void ProfileCheckSum(class GameStateClass*);
  slong CheckSum(GameStateClass*);
};

struct DesignStruct { 
  DesignStruct() {
    check = 0xAAAAAAAA;
    id = 0;
    raceID = 0;
    mass = 0;
    maxSpeed = 0;
    bitmapIndex = 0;
    expBitmapIndex = 0;
    expSoundIndex = -1;
    volumeCap = 0;
    name[0] = 0;
    checkSum = CheckSum();
  } 

  ulong checkSum;
  DEF_ADD_TO_STAT(short);
  DEF_SET_STAT(short);
  ulong CheckSum()
    {
      return volumeCap;
    }
  void ClassCheck()
    {
      return;
    }

  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short get(ifstream&, ObjectType, SFString*, SFString*, short);
  void put(ObjectType, class Document*);
  short HasFactory();
  short CompareTo(DesignStruct& d) {
    return
     id             == d.id             &&
     raceID         == d.raceID         &&
     mass	    == d.mass           &&
     maxSpeed	    == d.maxSpeed       &&
     bitmapIndex    == d.bitmapIndex    &&
     expBitmapIndex == d.expBitmapIndex &&
     volumeCap      == d.volumeCap      &&
     bus.CompareTo(d.bus);
  }
  slong TotalValue();
  /* data */
  slong check;
  slong id;
  slong raceID;
  slong mass;
  slong maxSpeed;
  short bitmapIndex;
  short expBitmapIndex;
  short expSoundIndex;
  short volumeCap;
  short objectType;
  BusIdStruct bus;
  char name[max_name_char];
};

int IsValidDesignStructPointer(DesignStruct *,char *,int,short=FALSE);

struct ShipDesignStruct : DesignStruct {
  ShipDesignStruct() : DesignStruct() {
    rotRate = 0;
    reqRank = 0;
    factory = 0;
    inFrame = 0;
    objectType = ship;
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short get(ifstream&, ObjectType, SFString*, SFString*, short);
  void put(ObjectType, class Document*);
  short operator==(ShipDesignStruct& s) {
    return reqRank == s.reqRank &&
      rotRate == s.rotRate &&
      factory == s.factory &&
      CompareTo(s);
  }
  /* data */
  slong  inFrame;
  short reqRank;
  char  rotRate;
  char  factory;
};

struct ColonyDesignStruct : ShipDesignStruct {
  ColonyDesignStruct() : ShipDesignStruct() {
    objectType = colony;
  }
};

struct MissileDesignStruct : DesignStruct {
  MissileDesignStruct() {
    warhead = 0;
    initialVelocityDisplacement = 0;
    actRange = 0;
    tracking = 0;
    trigger = 0;
    push = 0;
    camera = 0;
    seekSens = 0;
    objectType = missile;
  }
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  /* data */
  slong warhead;
  slong initialVelocityDisplacement;
  ulong actRange;
  short tracking;
  short trigger;
  short push;
  short vortex;
  char camera;
  char seekSens;
};

class MissileClass : public DesignedClass {
 public:
  friend struct BusStruct;
  friend class GameStateClass;
  friend class ModuleClass;
  friend class DesignedClass;
  MissileClass();
  MissileClass(short i, DesignedClass& g, MissileDesignStruct& md,
	       slong* par, slong altModID) {
    initializeWith(i, g, md, par, altModID);
  }
  void initializeWith(short, DesignedClass&, MissileDesignStruct&, slong* =0, slong =0);
  char* TypeName() const { return "missile"; }
  PingPacket& Ping(TargetType =ship_target,
		  TargetSelector =nearest, short =0,
		  ulong =pc_long_max,
		  CoordinateClass =nullVector);
  void Update();
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  void ShortProfile();
  void ProfileCheckSum(class GameStateClass*);
  slong CheckSum(GameStateClass*);
  void profile(GameStateClass*);
  void Acquire(ulong =pc_long_max);
  void Explode(char* ="no reason", short =1, short =0);
  void PlugInModules() {
    if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
      {
        PlugInModules((short)design->bus.numMod);
      }
  }
  void PlugInModules(short);
  void PlugInModuleByID(short, slong);
  void PlugInModuleByDesign(short j) {
    if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
      {
        PlugInModuleByID(j, design->bus.moduleIdList[j]);
      }
  }
  short IsExploding()
    {
      if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
	{
	  return bitmap.index == design->expBitmapIndex;
	}
      return 0;
    }
  short CanBeSeenBy(DesignedClass*);
  slong cX(slong scale =sub_scale, slong nFrames =0) { 
    return (now.x+tvel.x*nFrames)/scale;
  }
  slong cY(slong scale =sub_scale, slong nFrames =0) { 
    return (now.y+tvel.y*nFrames)/scale;
  }
  //private:
  char  targetLock;
  short triggerObjQualifier;
  short triggerObjType;
  short onImpact;
  MissileDesignStruct* design;
};

typedef MissileClass *MissileObject;

class ShipClass : public DesignedClass {
 public:
  friend class GameStateClass;
  friend class ModuleClass;
  enum AdviceType {standby, stand, flee};
  enum PendingFlag {spawning=1, docking=2, undocking=4, 
		    refitting=8, promotion=16, captured=32,
		    disappearing=64, activating_dock=128,
		    deactivating_dock=256, exploding=512, completing_build=1024,
		    tractoring=2048
  };
  ShipClass() {
    initialize();
  }
  /* ShipClass(short, ShipDesignStruct&, short); */
  ShipClass(short i, short pI, short numMod =0) {
    initializeWith(i, pI, numMod);
  }
  void initialize();
  void initializeWith(short, short, short =0);
  void SetPendingFlag(short mode) {
    pending = pending | mode;
  }
  void ClearPendingFlag(short mode) {
    pending = pending ^ (pending & mode);
  }
  short IsPending(short mode) {
    return short(pending & mode);
  }
  int IsAttachedToSB();
  void ShortProfile();
  void AssignToPlanet(short, short =0);
  short InitializeFromShipDesign(ShipClass*, short =0);
  short InitializeFromDesign(ShipClass*, short =0);
  void PlugInModuleByCopy(ModuleClass&);
  void SendGetPowerUpMsg(short, slong, slong);
  void ReceiveGetPowerUpMsg(short, slong, slong);
  short GetNextModuleIndexInUpgradePath(short, short =1, slong =0);
  short FindModuleToReplace(ModuleClass&,short&,short&,short,short,short,
			    short =1);
  short NewRotationRate(slong);
  void PlaceModule(ModuleClass&,short,short,short&, short =0);
  void SwapModuleByType(short);
  void DropModule() {
    DropModule(bus.modPtr);
  }
  void DropModule(char);
  void ToggleModuleMode();
  void Build(ShipClass* s, short port) {
    if ( IsValidObjectPointer(s) && bus.factoryMod ) {
      s->stat = under_construction;
      bus.factoryMod->workPar[0] = slong(s);
      bus.factoryMod->workPar[1] = 0;
      bus.factoryMod->specPar[3] = port;
      bus.factoryMod->activate();
      bus.factoryMod->Build();
    }
  }
  short get(ifstream&, SFString*, SFString*, short);
  short GetSharedData(ifstream&, ObjectType, SFString*, SFString*, short);
  slong Hit(slong, DesignedClass*, CoordinateClass&,
	   short =100, short =0, short =0);
  TargetType TType() const { return ship_target; }
  char* TypeName() const { return "ship"; }
  PingPacket& Ping(TargetType =planet_target,
		  TargetSelector =all,
		  short =0,
		  ulong =pc_long_max,
		  CoordinateClass =nullVector);
  short ShouldRefreshPing(PingPacket& pp, const char frameRate) {
    
    if (IsValidObjectPointer(pp.obj.generic,__FILE__,__LINE__)) {
      slong delayFrames = max(pp.dist / sub_scale / 20, 1);
      
      return (frame % delayFrames == 0);
    } else
      return (frame % frameRate == 0);
  }
  short WillHelp(ShipClass* s, short priority) {
    return (assign == manual ||
	    (assign == follow_object && targetPP.obj.generic != s)) &&
      priority >= assignPriority &&
      bus.aveFunc > ship_distress_func_level;
  }
  int IsStarbase()
    {
      return bus.numPorts > 1;
    }
  short CanConfirmHelp(GenericClass& g) {
    return duty == none || (*this - g).Mag(0) < (int)attack_dist;
  }
  void AssignDuty(short =0);
  void CheckDuty(short =0);
  void EnforceDuty() {
    if (Type() == ship && AutoResponse()) {
      ClearAssign();
      AssignDuty(1);
      CheckDuty(1);
    }
  }
  void ProcessAssignment(short&);
  short HasToAvoidStar() {

    if (IsWarping() || bus.warpMod && bus.warpMod->IsReady())
      return 0;

    static PingPacket starPP;
    starPP = Ping(star_target);
    slong h;
    
    /* the ship is close enough to take damage */
    
    if ( IsValidObjectPointer(starPP.obj.planet) )
      {
	slong dStar = starPP.dist;
	slong rStar = 2 * starPP.obj.planet->bitmap.size * sub_scale;
	
	if ( dStar < 3*rStar/2 && 100 * (rStar-dStar) / rStar)
	  {
	    if (starPP.obj.designed->IsLeftOf(this))
	      h = 1000;
	    else
	      h = -1000;
	    
	    if ( starPP.dist < 
		 2 * starPP.obj.planet->bitmap.size*(ulong)sub_scale)
	      FleeObj(starPP, 1000, h);
	    else
	      FleeObj(starPP, 0, h);
	    
	    return 1; 
	  }
      }
    return 0;
  }
  void Update();
  char* Query(ShipClass*, short =1, short =0);
  void profile(GameStateClass*);
  void ProfileCheckSum(class GameStateClass*);
  slong CheckSum(GameStateClass*);
  void SendSetTargetMsg(ObjectUnion);
  void SendRequestForPopulation();
  void SendConfirmationOfPopulation();
  void ReceiveConfirmationOfPopulation(short,slong, uchar, uchar, uchar, uchar, uchar, uchar,
				       slong, slong);
  void SendCaptureMsg(ShipClass*);
  void ReceiveCaptureMsg(ShipClass*, short);
  void ReceiveObjectTrackingMsg(class SF_Msg*, short =1);
  void SendObjectTrackingMsg();
  void SendSetModulePointerMsg();
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  short CheckPorts()
    {
      for ( int i=0; i<bus.numPorts; i++ )
	{
	  ModuleClass *dock = bus.dockMod[i];
	  if ( dock )
	    {
	      if ( dock->workPar[1] )
		{
		  ShipClass *dockee = (ShipClass *)dock->workPar[0];
		  if ( !IsValidObjectPointer(dockee) )
		    {
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  else if ( !IsValidObjectPointer(dockee->dockObj) )
		    {
		      dockee->dockObj = NULL;
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  else if ( !dockee->IsActive() )
		    {
		      dockee->dockObj = NULL;
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  else if ( dockee->dockObj != this )
		    {
		      dockee->dockObj = NULL;
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		}
	      if ( !dock->workPar[1] )
		{
		  ShipClass *dockee = (ShipClass *)dock->workPar[0];
		  if ( !IsValidObjectPointer(dockee) )
		    {
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  else if ( !dockee->IsActive() )
		    {
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  /*
		  else if ( !(assign == go_dock || assign == go_multi_dock) )
		    {
		      dockee->dockObj = NULL;
		      ProcessFreeDockMsg(bus.moduleList.indexOf(dock));
		    }
		  */
		}
	    }
	}
      return 0;
    }
  short FindNextFreePort() {
    short i = 0;
    CheckPorts();
    while (i < bus.numPorts && 
	   bus.dockMod[i]->workPar[1] &&
	   !bus.dockMod[i]->IsActive() )
      i++;
    return (i+1) % (bus.numPorts + 1);
  }
  void PlugInModules() {
    if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
      {
        PlugInModules((short)design->bus.numMod);
      }
  }
  void PlugInModules(short);
  void PlugInModuleByID(short, slong);
  void PlugInModuleByDesign(short j) {
    if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
      {
        PlugInModuleByID(j, design->bus.moduleIdList[j]);
      }
  }
  void Explode(char* ="no reason", short =1, short =0);
  void HandleExplodeMsg(short =1);
  void Acquire(ulong =pc_long_max);
  void AcquireNearPoint(CoordinateClass);
  void AcquireNext(TargetSelector =sequential, short =1);
  void AcquireNearestLeader(ulong range) {
    PingPacket nearPP = Ping(ship_target, nearest_leader, 0,
			     range,
			     nullVector);
    if (IsValidObjectPointer(nearPP.obj.generic,__FILE__,__LINE__) && 
	nearPP.dist < range) {
      targetPP.obj.ship = nearPP.obj.ship;
      Assign(follow_object);
    } else
      GoHome(attack);
  }
  void AssignNearestPlanet(short);

  short FreeDock(short port){
    ShipClass* s = (ShipClass*) bus.moduleList[port].workPar[0];
    if ( !IsValidObjectPointer(s) )
      {
	bus.moduleList[port].workPar[0] = 0;
	bus.moduleList[port].workPar[1] = 0;
	return 0;
      }
    s->SendUnDockMsg();
    return 1;
  }

  void ProcessFreeDockMsg(short);
  void ReceiveUnDockMsg() {
    ClearPendingFlag(undocking);
    if ( IsValidObjectPointer(dockObj) )
      {
	dockObj->ProcessFreeDockMsg(portNum);
      }
    dockObj = 0;
  }
  void SendUnDockMsg();
  void FreeFromDock(short force =1)
    {
      if ( !IsValidObjectPointer(dockObj) )
	{
	  dockObj = 0;
	  return;
	}
      
      if ( stat != under_construction || force )
	{
	  if ( dockObj->FreeDock(portNum) )
	    {
	      /* the dockObj will release this object */
	      //dockObj = 0;
	      viewObj.generic = this;
	    }
	  else
	    {
	      /* the dockObj does not know about this object       */
	      /* this is an error condition which should be logged */
	      dockObj = 0; /* sever the one-way link */
	    }
	}
    }

  void ReleaseDock(short);
  void ReleaseDocks() {
    for (short i = 0; i < bus.numPorts; i++) {
      ReleaseDock(i);
    }
  }
  void DeactivateDocks() {
    /* this will shut off all docks */
    for (short i = 0; i < bus.numPorts; i++) {
      ReleaseDock(i); /* to make sure they are empty */
      /* dock deactivation */
      bus.dockMod[i]->ReceiveDockDeactivationMsg();
    }
  }
  void AssignNextPlanet(short, TargetSelector =sequential, short =0);
  short IsExploding()
    {
      if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
	{
	  return bitmap.index == design->expBitmapIndex;
	}
      return 0;
    }
  short IsCloaked() {
    return stealth > 1 || stealthLevel > 0;
  }
  short IsAvailable() {
    return IsActive() && stat != under_construction;
  }
  short IsAvailableToHelp();
  slong Mass(slong delta =0, short include =1) const {

    slong tmass = DesignedClass::Mass(delta, include);
    
    if (include) {
      short i;
      for (i=0; i<bus.numPorts; i++)
	if (bus.dockMod[i]->workPar[1])
	  {
	    GenericClass *g = (GenericClass *)bus.dockMod[i]->workPar[0];
	    if ( IsValidObjectPointer(g) )
	      {
		tmass += g->mass;
	      }
	  }
    }
      
    /* mass from population / crew */
    tmass += pop.InUnitsOf(20000);

    /* mass from hull volume */
    if ( IsValidDesignStructPointer(design,__FILE__,__LINE__) )
      {
	tmass += design->volumeCap * hull_mass_per_unit_volume;
      }

    return tmass;

  }
  void BlockTubeFromFiring(short i) {
    tubeSelector = tubeSelector ^ (tubeSelector & (1 << i));
    bus.tubeMod[i]->deactivate();
  }
  void SetTubeToFire(short i) {
    tubeSelector = tubeSelector | (1 << i);
  }
  short IsTubeSetToFire(short i) {
    return tubeSelector & (1 << i);
  }
  void BlockDefensiveTubeFromFiring(short i) {
    defensiveTubeSelector = defensiveTubeSelector ^ (defensiveTubeSelector & (1 << i));
    bus.tubeMod[i]->deactivate();
  }
  void SetDefensiveTubeToFire(short i) {
    defensiveTubeSelector = defensiveTubeSelector | (1 << i);
  }
  short IsDefensiveTubeSetToFire(short i) {
    return defensiveTubeSelector & (1 << i);
  }
  enum advice {planet_to_colonize, planet_to_harvest};
  short PlanetAdvisor(advice =planet_to_colonize);
  short AttackAdvisor();
  void GadgetAdvisor(PingPacket&, short&, short);
  short CanBeSeenBy(DesignedClass*);
  short SendDistressSignal(short =0, short =0);
  void CalculateRemainingValueAndMass(slong&, slong&);
  void WageWar(ShipClass*);
  void AssignDesign(short);
  PopStruct OptimalCrew();
  short playerIndex;
  char videoMode;
  char lastVideoMode;
  /* private: */
  PingPacket avoidPP;
  ShipClass* dockObj;
  slong      distressFrame;
  slong blockadeFrames;
  ShipDesignStruct* design;
  PopStruct pop;
  short pending;
  short accuracy;
  short portNum;
  short stealthLevel;
  short tubeOffset[max_tubes_per_ship];
  short beamOffset[max_tubes_per_ship];
  char tubeSelector;
  char defensiveTubeSelector;
};

class ColonyClass : public ShipClass {
  /* each instance of this class holds information about a single */
  /* population of one race on one planet                         */
 public:
  ColonyClass() {
    initialize();
  }
  void initialize();
  short get(ifstream&, SFString*, SFString*, short);
  void put(class Document*);
  TargetType TType() const { return colony_target; }
  char* TypeName() const { return "colony"; }
  void ShortProfile();
  void ProfileCheckSum(class GameStateClass*);
  slong CheckSum(GameStateClass*);
  void profile(GameStateClass*);
  void Update();
  void AssignDesign(short);
  void AssignLocation(short);
  void Explode(char* ="no reason", short =1, short =0);
  char* Query(ShipClass*, short =1, short =0);
  slong Hit(slong, DesignedClass*, CoordinateClass&,
	   short =100, short =0, short =0);
  short InitializeFromColonyDesign(ColonyClass*, short =0);
  void ConvertConstructionProject();
  void TransferConstructionProjectTo(ColonyClass*);
  void SendSpawnMsg(PopStruct, short, short =-1, short =0);
  short Spawn(PopStruct, short, short =-1, short =-1);
  void SpawnToDock(ShipClass* s);
  slong Loyalty();
  inline slong Crowding();
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  /*private:*/
  char      name[max_name_char];
  short     planetIndex;
  short     lastDockObjIndex;
  short     supply;
  slong      kills;
  
};
  
struct ListStruct {
  char* list;
  short nTotal;
  short nSet;
  ListStruct() {
    InitializeWith(0);
  }
  ListStruct(short n) {
    InitializeWith(n);
  }
  ~ListStruct() {
    if (list) {
      delete list;
      list = 0;
    }
  }
  void ReInitializeWith(short n) {
    if (list) {
      delete list;
      list = 0;
    }
    InitializeWith(n);
  }
  void InitializeWith(short n) {
    nTotal = n;
    nSet = 0;
    if (n) 
      list = new char[n];
    else
      list = 0;

    short i;
    for (i=0; i<n; i++)
      list[i] = 0;
  }
  short IsSet(short index) {
    if ( list && index >= 0 && index < nTotal )
      {
	return list[index];
      }
    return 0;
  }
  short set(short index) {
    if (list && index >= 0 && index < nTotal) {
      if (list[index])
	return 0;
      else {
	nSet++;
	list[index] = 1;
	return 1;
      }
    } else
      return 0;
  }
  short get(short index) {
    if (!list || index > nSet && index < nTotal )
      return 0;
    short i = 0;
    short j = 0;
    while (j < index && i < nTotal) {
      j += (list[i]>0);
      i++;
    }
    if (i < nTotal)
      return i;
    else
      return 0;
  }
  void copy(ListStruct& l, short n =0) {
    if (n)
      ReInitializeWith(n);
    else {
      n = l.nTotal;
      if (nTotal != l.nTotal)
	ReInitializeWith(l.nTotal);
    }
    nSet = l.nSet;
    nTotal = l.nTotal;
    if (list == 0)
      list = new char[n];
    short i;
    for (i=0; i<n; i++)
      list[i] = l.list[i];
  }
  void CopyTo(ListStruct& l, short n =0) {
    if (n == 0)
      n = l.nTotal;
    nSet = l.nSet;
    nTotal = l.nTotal;
    if (list == 0)
      list = new char[n];
    short i;
    for (i=0; i<n; i++)
      list[i] = l.list[i];
  }
  DEF_OBJ_PUSHTO;
  DEF_OBJ_POPFROM;
};
    

class GameStateClass {
  /* holds entire game state -- the unit of exchange during
     network logins */
 public:
  friend class ShipClass;
  friend class MissileClass;
  friend class DesignedClass;
  friend class ModuleClass;
  friend class PlayerClass;
  friend class ColonyClass;
  friend class GameStateBuffer;
  enum State {active=1, pending=2, demotions=4, balance=8, defaults_applied=16,
	      autoSpawn=32, laserDefense=64, betweenFrames=128};
  enum CountType {ships, planets, statics, missiles, objects, bitmaps,
		  ship_designs, missile_designs, races, stored_players,
		  robot_players, human_ships, colonies, modules,
		  first_ship, last_ship, max_colonies,
		  first_planet, last_planet, first_colony, last_colony};
  enum powerup_action {swap, increment_level_upgrade,
		       increment_pct_value_upgrade,
		       increment_pct_credit_upgrade};
  GameStateClass();
  GameStateClass(short, short, short, short, short, short, short, short,
		 short, short, short, short, short, short, short, short,
		 short, short, short, short, short, short);
  ~GameStateClass() {
    ClearCurrentState();
  }
  void ClearCurrentState();
  void DestroyCurrentState();
  void InitializeWith(short, short, short, short, short, short, short, short,
		 short, short, short, short, short, short, short, short,
		 short, short, short, short, short, short);
  short N(CountType);
  short NumObjects() {
    return numShips + maxPlanets + maxColonies + numMissiles;
  }
  short MaxObjects() {
    return numShips + maxPlanets + maxColonies + maxMissiles;
  }
  short FirstPlanetObjectIndex() {
    return planetObjectIndexStart;
  }
  short FirstColonyObjectIndex() {
    return colonyObjectIndexStart;
  }
  short FirstMissileObjectIndex() {
    return missileObjectIndexStart;
  }
  short NumModulesByType(short, short);
  void WriteData(class Document*);
  short ReadData(ifstream&, SFString* =0, SFString* =0, short =0);
  short ExtendRace(RaceClass&);
  short AddRace(ifstream&, SFString* =0, SFString* =0, short =0);
  short ExpandPlayer();
  short ExtendPlayer(PlayerClass&);
  short AddPlayer(ifstream&, SFString* =0, SFString* =0, short =0);
  PlayerClass* AddPlayer();
  void RemovePlayer(short i);
  PlayerClass* FindPlayer(char*, char*);
  short AddSound(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddSoundEvent(ifstream&, SFString* =0, SFString* =0, short =0);
  short ExtendBitmap(GameBitmapClass&);
  short AddBitmap(ifstream&, char*, SFString* =0, SFString* =0, short =0);
  short AddBitmap(GameBitmapClass gbmp) {
    return ExtendBitmap(gbmp);
  }
  short AddShipDesign(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddColonyDesign(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddMissileDesign(ifstream&, SFString* =0, SFString* =0, short =0);
  short ExpandPlanet(PlanetClass* =0);
  short ExtendPlanet(PlanetClass&);
  short AddPlanet(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddModule(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddRank(ifstream&, SFString* =0, SFString* =0, short =0);
  short ExtendColonyDesign(ColonyDesignStruct&);
  short ExpandColony();
  short ExtendColony(ColonyClass&);
  short AddShip(ifstream&, SFString* =0, SFString* =0, short =0);
  short AddColony(ifstream&, SFString* =0, SFString* =0, short =0);
  ShipClass* FindFreeFactory();
  ShipClass* FindFreeFactory(slong);
  short FindFreePlayerWithFactoryIndex(short, slong);
  short FindNextFreePlayerIndex(short);
  short FindNextFreePlayerIndex(short, slong);
  short FindNextShipTypeIndex(slong, short, short, short =0);
  short FindNextShipTypeIndexAtLevel(slong, short, short, short =0);
  slong IDOfModule(const char*, const slong =-1) const;
  short IndexOfSound(const char*, const short objType = all_type, const slong =-1) const;
  short IndexOfPlanet(const char*) const;
  short IndexOfBitmap(const char*, const short, const slong =-1) const;
  short IndexOfPlayer(const char*) const;
  short IndexOfRace(const char*) const;
  short IndexOfDesign(const char*, const ObjectType, const slong) const;
  short LookUpMissileDesignId(slong);
  short LookUpID(ColonyDesignStruct*);
  short LookUpModuleById(slong id, ostream* err =0) const {
    short i;
    for (i=0; i<numModules; i++) {
      if (id == moduleList[i].id)
	return i;
    }
    if (err)
      *err << "ERROR> LookUpModuleById: unknown module id: " <<
	id << endl;
    return 0;
  }
  short HasValidObjectList(char* location) {
    short i, n;
    ObjectUnion obj;
    short error = 0, newError;
    for (i = shipObjectIndexStart; i <= shipObjectIndexEnd; i++) {
      if ( IsValidObjectPointer(objectList[i]) && objectList[i]->active) {
	obj.generic = objectList[i];
	newError = !obj.designed->IsValidDesignedObject(i, location);
	error = error || newError;
      }
    }
    for (i = colonyObjectIndexStart; i <= colonyObjectIndexEnd; i++) {
      if (IsValidObjectPointer(objectList[i]) && objectList[i]->active) {
	obj.generic = objectList[i];
	newError = !obj.designed->IsValidDesignedObject(i, location);
	error = error || newError;
      }
    }
    n = min(missileObjectIndexEnd, missileObjectIndexStart + numMissiles - 1);
    for (i = missileObjectIndexStart; i <= n; i++) {
      if (IsValidObjectPointer(objectList[i]) && objectList[i]->active) {
	obj.generic = objectList[i];
	newError = !obj.designed->IsValidDesignedObject(i, location);
	error = error || newError;
      }
    }
    return !error;
  }
  unsigned short BonusPctOf(PlayerClass* p) {
    return (unsigned short)(p->strategicPoints * 50 / 
			    max(race[p->raceId].strategicPoints, 1) +
			    p->activeFrames * 50 / 
			    max(race[p->raceId].activeFrames, 1));
  }
  short GetMaxPopRatioPlanetIndex(slong);
  void BalanceNewPlayerOnServer(short);
  short Random(short n, slong seed =0) {
    if ( !numPlanets || !n ) return 0;
    GenericClass* g = planet  + ((short)(frame+n) % numPlanets);
    if ( IsValidObjectPointer(g) )
      {
	return (short)(labs(g->frame + g->now.x + g->now.y+seed + randomSeed) % n);
      }
    return 0;
  }
  char* PlayerRankName(short pI) {
    return rank[min(player[pI].rank,numRanks-1)].name;
  }
  short CreateColonyOnPlanet(short, short, PopStruct, short =-1);
  short PlanetIsSpawnable(short planetIndex, GameStateClass* g) {
    return PlanetIsSpawnable(planet + planetIndex, g);
  }
  short PlanetIsSpawnable(PlanetClass* p, GameStateClass* g) {
    if ( IsValidObjectPointer(p) && IsValidObjectPointer(p->colony) )
      return p->colony->pop.InUnitsOf(100) /
	max(p->OptimalPopulation(race[p->raceID]).InUnitsOf(10000),1) >
	50 &&
	numColonies < maxColonies &&
	!p->colony->IsPending(ShipClass::spawning) &&
	!g->colonyPending;
    return 0;
  }
  void SetDefaults();
  void ApplyModuleDefaults();
  void RespondToLastDistress(ShipClass* s) {

    if ( IsValidObjectPointer(s) &&
	 IsValidObjectPointer(race[s->raceID].distressObj.generic) )
      s->FollowObj(race[s->raceID].distressObj.generic);

  }
  void BlastObjects(DesignedClass*, slong, slong, short, short);
  void ExchangeShips(short);
  void PostGameMessage(char*, short, short, short, short=0);
  void CheckRank(short pI, ShipClass* s, short build =0) {
    if ( IsValidObjectPointer(s) )
      {
	/* refit ship if player's ship does not match player's race or */
	/* player's rank is greater than required for current ship     */
	if (player[pI].raceId != s->design->raceID ||
	    player[pI].rank < s->design->reqRank ||
	    s->design->inFrame > frame)
	  DecideToRefitShip(s - ship.rootOf(), 1, build);
	//OldRefitShip(s - ship, 1, build);
      }
  }
  void MarkFactoryDesigns() {
    short i;
    for (i=0; i < numShipDesigns; i++) 
      shipDesign[i].factory = (char)shipDesign[i].HasFactory();
  }
  void ProfileModuleList();
  void ProfileObjectList();
  void MapStaticObjectList() {

    /* remove old list */
    objectList.destroy();

    /* update index markers */
    shipObjectIndexStart = 0 /*maxPlanets*/;
    shipObjectIndexEnd = shipObjectIndexStart + numShips - 1;
    planetObjectIndexStart = shipObjectIndexEnd + 1 /*0*/;
    planetObjectIndexEnd = planetObjectIndexStart + maxPlanets - 1;
    colonyObjectIndexStart = planetObjectIndexEnd + 1;
    colonyObjectIndexEnd = colonyObjectIndexStart + maxColonies - 1;
  
    /* create new list */
    objectList.initializeWith(MaxObjects());

    /* map ships */
    short i;
    for (i = shipObjectIndexStart; i <= shipObjectIndexEnd; i++)
      objectList[i] = &ship[i-shipObjectIndexStart];
    
    /* map planets */
    for (i = planetObjectIndexStart; i <= planetObjectIndexEnd; i++)
      objectList[i] = &planet[i-planetObjectIndexStart];

    /* map colonies */
    for (i = colonyObjectIndexStart; i <= colonyObjectIndexEnd; i++)
      objectList[i] = &colony[i-colonyObjectIndexStart];

  }
  void MapObjectListFromMissileList() {
    /* build the objectList from an existing missileList */
    /* this is needed within GameStateClass::UnPack()    */
    MapStaticObjectList();

    missileObjectIndexStart = colonyObjectIndexEnd + 1;
    missileObjectIndexEnd = missileObjectIndexStart + maxMissiles - 1;
    
    short i;
    for (i=0; i < numMissiles; i++)
      objectList[i + missileObjectIndexStart] = missileList[i];

  }
  void MapObjectList() {

    MapStaticObjectList();

    UpdateMissileList();
  }
  void DecideToRefitShip(short, short =0, short =0, ShipClass* =0);
  void RefitShip(short, short, short =0, short =0, ShipClass* =0);
  void OldRefitShip(short, short =0, short =0);
  void RandomizeGameState(slong);
  void CheckProximityToStars(DesignedClass*);
  void Defect(short);
  /* GenericClass* GetViewObj(DesignedClass* d) { */
  /*     for (short i = 0; i < numMissiles; i++) */
  /*       if (missileList[i]->IsFriendlyTo(d) && */
  /* 	  missileList[i]->design->camera) */
  /* 	return missileList[i]; */
  /*     return 0; */
  /*   } */
  GenericClass* GetNextViewObj(DesignedClass* d, short dir) {
    short nObj = N(objects);
    short start = 0;
    if ( IsValidObjectPointer(d) && IsValidObjectPointer(d->viewObj.generic) )
      start = (ObjectListIndex(d->viewObj.generic) + dir + nObj) % nObj;
    short j = 0;
    short i;
    for (i = start; j < nObj; i += dir) {
      j++;
      if (IsValidObjectPointer(objectList[(i + nObj) % nObj]) &&
	  objectList[(i + nObj) % nObj]->IsActive() &&
	  objectList[(i + nObj) % nObj]->IsFriendlyTo(d) &&
	  (objectList[(i + nObj) % nObj]->Type() == ::ship ||
	   (objectList[(i + nObj) % nObj]->Type() == ::missile &&
	    ((MissileClass*)objectList[(i + nObj) % nObj])->design->camera)))
	return objectList[(i + nObj) % nObj];
    }
    return 0;
  }
  ObjectUnion ObjectListObj(short i) {
    ObjectUnion obj;
    if (i >= 0 && i <= missileObjectIndexEnd) {
      if (i <= shipObjectIndexEnd)
	obj.ship = ship + (i - shipObjectIndexStart);
      else if (i <= planetObjectIndexEnd)
	obj.planet = planet + (i - planetObjectIndexStart);
      else if (i <= colonyObjectIndexEnd)
	obj.colony = colony + (i - colonyObjectIndexStart); /* renum */
      else
	obj.missile = missileList[i - missileObjectIndexStart];
    } else /* unknown */
      obj.objLong = 0;

    return obj;
  }
  short ObjectListSafeIndex(GenericClass* p) {
  return 0;
  }
  short ObjectListIndex(GenericClass* p) {

    if ( !IsValidObjectPointer(p) ) return -1;

    short i;
    switch (p->Type()) {
    case ::generic:
    case ::planet:
      return planetObjectIndexStart + planet.indexOf((PlanetClass*)p);
    case ::ship:
      return shipObjectIndexStart + ship.indexOf((ShipClass*)p);
    case ::colony:
      return colonyObjectIndexStart + colony.indexOf((ColonyClass*)p); /* renum */
    case ::missile:
      for (i=0; i<numMissiles; i++) {
	if (missileList[i] == p)
	  return missileObjectIndexStart + i;
      }
      return missileObjectIndexStart;
    default:
      return 0;
    }
  }
  short ObjectListIndex(PlanetClass* p) {

    if (!p) return -1;

    return planetObjectIndexStart + p - planet.rootOf();
  }
  short ObjectListIndex(ShipClass* p) {

    if ( !IsValidObjectPointer(p) ) return -1;

    switch (p->Type()) {
    case ::ship:
      return shipObjectIndexStart + ship.indexOf(p);
    case ::colony:
      return colonyObjectIndexStart + colony.indexOf((ColonyClass*)p);
    default :
      break;
    }
    /* should never happen */
    return 0L;
  }
  short ObjectListIndex(ColonyClass* p) {

    if (!p) return -1;

    return colonyObjectIndexStart + colony.indexOf(p);
  }
  short ObjectListIndex(MissileClass* p) {

    if (!p) return -1;

    short i;
    for (i=0; i<numMissiles; i++) {
      if (missileList[i] == p)
	return missileObjectIndexStart + i;
    }
    return missileObjectIndexStart;
  }
  short TypeListIndex(GenericClass* p) {

    if ( !IsValidObjectPointer(p) ) return -1;

    switch (p->Type()) {
    case ::planet:
      return planet.indexOf((PlanetClass*)p);
    case ::ship:
      return ship.indexOf((ShipClass*)p);
    case ::colony:
      return colony.indexOf((ColonyClass*)p);
    case ::missile:
      return missile.indexOf((MissileClass*)p);
    default:
      return 0;
    }
  }
  short TypeListIndex(PlanetClass* p) {

    if (!p) return -1;

    return p - planet.rootOf();
  }
  short TypeListIndex(ShipClass* p) {

    if ( !IsValidObjectPointer(p) ) return -1;

    switch (p->Type()) {
    case ::ship:
      return p - ship.rootOf();
      break;
    case ::colony:
      return colony.indexOf((ColonyClass*)p);
      break;
    default :
      break;
    }
    return 0;
  }
  short TypeListIndex(ColonyClass* p) {

    if (!p) return -1;

    return colony.indexOf(p);
  }
  short TypeListIndex(MissileClass* p) {

    if (!p) return -1;

    return p - missile.rootOf();
  }
  short RaceIdToIndex(slong id) {
    short i = 0;
    while(race[i].id != id)
      i++;
    return i;
  }
  short IsRobotPlayer(ShipClass* s) {
    if ( IsValidObjectPointer(s) )
      {
	return s->Type() == ::colony || s->playerIndex < numRobotPlayers;
      }
    return 0;
  }
  short MapPlayerShipIdToIndex(short);
  short ValidateModuleID(slong id) {
    short k;
    for (k = 0; k < numModules; k++) 
      if (id == moduleList[k].id)
	/* found */
	return k;

    /* failed to find */
    return -1;
  }
  void PlugInModules();
  void CreateLoadedModuleList();
  void DisperseLoadedModuleList();
  void DestroyLoadedModuleList() {
    loadedModuleList.destroy();
  }
  short NewObject(MissileClass**);
  short NewObject(ColonyClass**, short =-1);
  void DestroyObject(MissileClass*);
  void SendDestroyObjectMsg(ColonyClass*);
  void ReceiveDestroyObjectMsg(ColonyClass*, short =1);
  void Recycle();
  void Update();
  void UpdateDockedObjects();
  void UpdateDockedObject(ShipClass*);
  void UpdateMissileList();
  void FillSlot(short, short, short, short, ShipClass*, short);
  void WriteFile(int, char*);
  slong Size();
  DEF_OBJ_PUSHTO;
  DEF_OBJ_POPFROM;
  void InitializeTechList();
  void CleanOrphanObjects(ShipClass*);
  void DetonateMissilesOf(ShipClass*);
  short SummonFleetTo(ShipClass*, short);
  void BindFleetTo(ShipClass* p, GenericClass* s) {
    for (short i = 0; i < numShips; i++) {
      if (ship[i].assign == DesignedClass::follow_object &&
	   ship[i].targetPP.obj.ship == p) {
	BindShipTo(ship+i, s);
      }
    }
  }
  void ReleaseFleetFrom(ShipClass*);
  void GenericRelease(ShipClass*);
  void SummonShipTo(ShipClass*, short);
  void BindShipTo(ShipClass*p, GenericClass* s)
    {
      if ( IsValidObjectPointer(p) )
	{
	  if ( IsValidObjectPointer(s) )
	    {
	      p->dutyPar = ObjectListIndex(s);
	    }
	  p->duty = defend_detail;
	}
    }
  void SendStealthCommandToFleet(GenericClass*);
  void SendCommandToFleet(GenericClass*, short);
  void SendCommandToFleet(GenericClass*, short, CoordinateClass);
  void SendAssignToFleet(GenericClass*, DesignedClass::assignment, 
			 slong =0);
  short PlayerIsUpgradeable(short);
  short validate();
  void Profile();
  void ProfileByRace(ostream&);
  short IsActive() {
    return mode & active;
  }
  void activate() {
    mode = mode | active;
  }
  void deactivate() {
    mode = mode ^ (mode & active);
  }
  short IsPending() {
    return mode & pending;
  }
  void SetPendingFlag() {
    mode = mode | pending;
  }
  void ClearPendingFlag() {
    mode = mode ^ (mode & pending);
  }
  void SetRankDemotions() {
    mode = mode | demotions;
  }
  void ClearRankDemotions() {
    mode = mode ^ (mode & demotions);
  }
  short RanksAreDemoteable() {
    return mode & demotions;
  }
  void SetServerBalance() {
    mode = mode | balance;
  }
  void ClearServerBalance() {
    mode = mode ^ (mode & balance);
  }
  short ServerIsBalanced() {
    return mode & balance;
  }
  short DefaultsAreApplied() {
    return mode & defaults_applied;
  }
  void SetDefaultsApplied() {
    mode = mode | defaults_applied;
  }
  void SetMode(char m) {
    mode = mode | m;
  }
  void ClearMode(char m) {
    mode = mode ^ (mode & m);
  }
  short ModeIsSet(char m) {
    return mode & m;
  }
  /* public data */
  //ShipClass* ship;
  Array<ShipClass, short> ship;
  //PlanetClass* planet;
  Array<PlanetClass, short> planet;
  //MissileClass* missile;
  Array<MissileClass, short> missile;
  //ShipDesignStruct* shipDesign;
  Array<ShipDesignStruct, short> shipDesign;
  //MissileDesignStruct* missileDesign;
  Array<MissileDesignStruct, short> missileDesign;
  ColonyDesignStruct* colonyDesign;
  //MissileClass** missileList;
  Array<MissileClass*,short> missileList;
  //GenericClass** objectList;
  Array<GenericClass*,short> objectList;
  //GameBitmapClass* objectBitmapList;
  Array<GameBitmapClass, short> objectBitmapList;
  //SoundClass* sound;
  Array<SoundClass, short> sound;
  //ModuleClass* moduleList;
  Array<ModuleClass, short> moduleList;
  //ModuleClass* loadedModuleList;
  Array<ModuleClass, short> loadedModuleList;
  Array<RaceClass,slong> race;
  //PlayerClass* player;
  Array<PlayerClass, short> player;
  //PlayerClass** playerList;
  Array<PlayerClass*, short> playerList;
  //RankStruct* rank;
  Array<RankStruct, short> rank;
  //ColonyClass* colony;
  Array<ColonyClass, short> colony;
  //ColonyClass** colonyList;           
  Array<ColonyClass*, short> colonyList;
  char scenarioName[128];
  char scenarioDesc[8192];
  char fileName[128];
  short shipObjectIndexStart;
  short planetObjectIndexStart;
  short colonyObjectIndexStart;
  short missileObjectIndexStart;
  short shipObjectIndexEnd;
  short planetObjectIndexEnd;
  short colonyObjectIndexEnd;
  short missileObjectIndexEnd;
  short shipBitmapIndexStart;
  short shipBitmapIndexEnd;
  short planetBitmapIndexStart;
  short planetBitmapIndexEnd;
  short colonyBitmapIndexStart;
  short colonyBitmapIndexEnd;
  short missileBitmapIndexStart;
  short missileBitmapIndexEnd;
  short explosionBitmapIndexStart;
  short explosionBitmapIndexEnd;
  short transitionBitmapIndexStart;
  short debrisBitmapIndexStart;
  short miscellaneousBitmapIndexStart;
  short lastPlanet;
  short lastStar;
  short balanceMaxGap;
  short bonusFactor;
  short salvageRightPct;
  short bountyRightPct;
  short surrenderAllowed;
  short landRightFactor;
  short AILevel;
  short soundEvent[last_event];
  slong  year;
  slong  frame;
  slong  lightSpeed;
  slong  totPop;
  slong  randomSeed;
  slong  defectionRate;
  slong  loyaltyCycle;
  slong  offensiveFactor;
  slong maxColonyMillions;
  /* private: */
  short numShips;
  short numPlanets;
  short numMissiles;
  short numBitmaps;
  short numActiveShips;
  short numMissilesDestroyed;
  short numShipDesigns;
  short numMissileDesigns;
  short numModules;
  short numRaces;
  short numStoredPlayers;
  short numRobotPlayers;
  short numRanks;
  short numColonies;
  short numColoniesDestroyed;
  short numColoniesCreated;
  short numColonyDesigns;
  short numSounds;
  short maxPlanets;
  short maxMissiles;
  short maxBitmaps;
  short maxShipDesigns;
  short maxMissileDesigns;
  short maxModules;
  short maxRaces;
  short maxStoredPlayers;
  short maxHumanShips;
  short maxColonies;
  short extraColonies;
  short lastMissile;
  short maxAcc;
  short maxAngles;
  short planetShipGFactor;
  short planetPlanetGFactor;
  short damageToMass;
  short popPercentPerRobot;
  short planetsPerRobot;
  short numPopRobots;
  short numPlanetRobots;
  short debrisCount;
  short colonyPending;
  short pluginAction;
  short pluginPar;
  /* temporary variables -- do not need to be transferred */
  short numLoadedModules;
  char mode;
  ListStruct* tech;
};

union DataUnion {
  char           *c_;
  unsigned char  *cu;
  signed char    *cs;
  short          *s_;
  unsigned short *su;
  signed short   *ss;
  int            *i_;
  unsigned int   *iu;
  signed int     *is;
  slong           *l_;
  ulong  *lu;
  slong    *ls;
};

/* define network - hardware converters that are missing */

#define htonc(c) c
#define htoni htonl
#define ntohc(c) c
#define ntohi ntohl

/* define a function for each type to push its data into the buffer */
#define DEF_PUSH_DATA(g,t,d,m) slong Push(t x##d##m) {*g.d##m = hton##d(x##d##m); g.d##m++; return sizeof(t);}
/* define a function for each type to pop its data out of the buffer */
#define DEF_POP_DATA(g,t,d,m) slong Pop(t& x##d##m) {x##d##m = ntoh##d(*g.d##m); g.d##m++; return sizeof(t);}
#define DEF_PUSH_OBJ(t) slong Push(t& x, GameStateClass* gm) {return x.PushTo(*this, gm);}
#define DEF_POP_OBJ(t) slong Pop(t& x, GameStateClass* gm) {return x.PopFrom(*this, gm);}
#define DEF_PUSH_GAME(t) slong Push(t& x) {return x.PushTo(*this);}
#define DEF_POP_GAME(t) slong Pop(t& x) {return x.PopFrom(*this);}

class GameStateBuffer {
 public:
  GameStateBuffer() {
    InitializeWith(0);
  }
  GameStateBuffer(char* c) {
    InitializeWith(c);
  }
  void InitializeWith(char* c) {
    root = c;
    g.c_ = c;
  }
  char* BufferPosition() {
    return g.c_;
  }
  slong Size() {
    return g.c_ - root;
  }
  DEF_PUSH_DATA(g,char,c,_);
  DEF_PUSH_DATA(g,unsigned char,c,u);
  DEF_PUSH_DATA(g,signed char,c,s);
  DEF_PUSH_DATA(g,short,s,_);
  DEF_PUSH_DATA(g,unsigned short,s,u);
  //DEF_PUSH_DATA(g,signed short,s,s);
  DEF_PUSH_DATA(g,slong,l,_);
  DEF_PUSH_DATA(g,ulong,l,u);
  //DEF_PUSH_DATA(g,signed long,l,s);
#ifndef SFHOST  /* port 2026-10-12: int == slong (both 32-bit, same bytes) */
  DEF_PUSH_DATA(g,int,i,_);
  DEF_PUSH_DATA(g,unsigned int,i,u);
#endif
  //DEF_PUSH_DATA(g,signed int,i,s);
  DEF_POP_DATA(g,char,c,_);
  DEF_POP_DATA(g,unsigned char,c,u);
  DEF_POP_DATA(g,signed char,c,s);
  DEF_POP_DATA(g,short,s,_);
  DEF_POP_DATA(g,unsigned short,s,u);
  //DEF_POP_DATA(g,signed short,s,s);
  DEF_POP_DATA(g,slong,l,_);
  DEF_POP_DATA(g,ulong,l,u);
  //DEF_POP_DATA(g,signed long,l,s);
#ifndef SFHOST  /* port 2026-10-12: int == slong (both 32-bit, same bytes) */
  DEF_POP_DATA(g,int,i,_);
  DEF_POP_DATA(g,unsigned int,i,u);
#endif
  //DEF_POP_DATA(g,signed int,i,s);
  DEF_PUSH_OBJ(ShipClass);
  DEF_POP_OBJ(ShipClass);
  DEF_PUSH_OBJ(ColonyClass);
  DEF_POP_OBJ(ColonyClass);
  DEF_PUSH_OBJ(PlanetClass);
  DEF_POP_OBJ(PlanetClass);
  DEF_PUSH_OBJ(MissileClass);
  DEF_POP_OBJ(MissileClass);
  DEF_PUSH_OBJ(GameBitmapClass);
  DEF_POP_OBJ(GameBitmapClass);
  DEF_PUSH_OBJ(ObjectBitmapClass);
  DEF_POP_OBJ(ObjectBitmapClass);
  DEF_PUSH_OBJ(ShipDesignStruct);
  DEF_POP_OBJ(ShipDesignStruct);
  DEF_PUSH_OBJ(MissileDesignStruct);
  DEF_POP_OBJ(MissileDesignStruct);
  DEF_PUSH_OBJ(ModuleClass);
  DEF_POP_OBJ(ModuleClass);
  DEF_PUSH_OBJ(RaceClass);
  DEF_POP_OBJ(RaceClass);
  DEF_PUSH_OBJ(PlayerClass);
  DEF_POP_OBJ(PlayerClass);
  DEF_PUSH_OBJ(RankStruct);
  DEF_POP_OBJ(RankStruct);
  DEF_PUSH_GAME(ListStruct);
  DEF_POP_GAME(ListStruct);
  DEF_PUSH_GAME(CoordinateClass);
  DEF_POP_GAME(CoordinateClass);
  DEF_PUSH_OBJ(PingPacket);
  DEF_POP_OBJ(PingPacket);
  DEF_PUSH_OBJ(BusStruct);
  DEF_POP_OBJ(BusStruct);
  DEF_PUSH_OBJ(BusIdStruct);
  DEF_POP_OBJ(BusIdStruct);
  DEF_PUSH_GAME(PopStruct);
  DEF_POP_GAME(PopStruct);
  DEF_PUSH_OBJ(SoundClass);
  DEF_POP_OBJ(SoundClass);
  DEF_PUSH_GAME(GameStateClass);
  DEF_POP_GAME(GameStateClass);
  slong Push(char* xstr) {
    slong l = strlen(xstr);
    //logFile << "(" << xstr << ")" << endl;
    strcpy(g.c_, xstr);
    /* move the pointer over the length of the SFString */
    g.c_ += l;
    /* null terminate the SFString */
    *g.c_ = '\0';
    /* move the pointer again */
    g.c_++;

    return l + 1;
  }
  slong Pop(char* xstr) {
    slong l = strlen(g.c_);

    strcpy(xstr, g.c_);
    /* move the pointer over the length of the SFString + 1 for null */
    g.c_ += l + 1;

    return l + 1;
  }
  slong Push(GenericClass*, GameStateClass*);
  slong Pop(GenericClass*&, GameStateClass*);
  slong Pop(ShipClass*& s, GameStateClass* gm) {
    ObjectUnion gs;
    gs.ship = s;
    slong size = Pop(gs.generic, gm);
    s = gs.ship;
    return size;
  }
  slong Pop(ColonyClass*& s, GameStateClass* gm) {
    ObjectUnion gs;
    gs.colony = s;
    slong size = Pop(gs.generic, gm);
    s = gs.colony;
    return size;
  }
 private:
  DataUnion g;
  char* root;
};
/*** Functions ***/

char sequence();

void InitDir();

slong FrameNumber( void );

void PushMessage( char * );
void PushMessageNew( uchar,uchar,char * );

#ifdef SF_MAIN
/* global structures or objects */
GameStateClass* game = NULL;
#else
extern GameStateClass* game;
#endif

#if defined(SF_DEVMODE)
#define WRITE_SF_LOG(line) line;
#else
#define WRITE_SF_LOG(line)
#endif
