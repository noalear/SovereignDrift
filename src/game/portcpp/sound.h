/* === Stellar Frontier port: MODIFIED FILE ============================== */
/* This file has been changed from the original Stardock Shared Source  */
/* release of Stellar Frontier, for the port to modern compilers and    */
/* operating systems.  It is not the original Software.                 */
/* Changed on: 2026-10-12                                                */
/* ======================================================================= */
class SoundClass {
  /* game sounds and associated characteristics */
 public:
  friend class GameStateClass;
  SoundClass();
  void initialize();
  short get(ifstream&, ostream* =0);
  void put(class Document*) const;
  DEF_GAMEOBJ_PUSHTO;
  DEF_GAMEOBJ_POPFROM;
  //private:
  slong raceID;
  char name[max_name_char];
  char file[max_name_char];  
  short priority;
  short volume;
  short objectType;
  char loop;
};
