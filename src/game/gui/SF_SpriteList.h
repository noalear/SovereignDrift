#ifndef SF_SPRITELIST_INCLUDED
#define SF_SPRITELIST_INCLUDED

class SF_Sprite;

class SF_SpriteList
{
 public:
  
  ulong check;
  
  int numSprites;
  SF_Sprite **sprite;
  
  int Add(SF_Sprite *);
  
  SF_Sprite *Sprite(int);
  SF_Sprite *Sprite(char *);
  
  int Remove(int);
  int Remove(char *);
  int RemoveAll();
  
  int Delete(int);
  int Delete(char *);
  int DeleteAll();
  
  SF_SpriteList();
  ~SF_SpriteList();
};

#endif
