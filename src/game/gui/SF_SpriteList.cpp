#include "sf.h"
#include "SF_SpriteList.h"
#include "SF_Sprite.h"

SF_SpriteList::SF_SpriteList()
{
  check = 0xAAAAAAAA;
  numSprites = 0;
  sprite = NULL;
}

SF_SpriteList::~SF_SpriteList() {
   if ( sprite ) {
      S_free(sprite);
      sprite = NULL;
      }
   }


int
SF_SpriteList::Add(SF_Sprite *newSprite) {
   if ( numSprites == 0 && sprite == NULL )
      sprite = (SF_Sprite **)S_malloc(sizeof(SF_Sprite *));
   else if ( numSprites > 0 && sprite != NULL )
      sprite=(SF_Sprite **)S_realloc(sprite,sizeof(SF_Sprite *)*(numSprites+1));

   sprite[numSprites] = newSprite;
   numSprites++;
   return 0;
   }

SF_Sprite *
SF_SpriteList::Sprite(int i) {
   if ( i<0 || i>=numSprites )
      return NULL;
   return sprite[i];
   }


SF_Sprite *
SF_SpriteList::Sprite(char *filename) {
   SF_Sprite *foundSprite = NULL;
   for ( int i=0; i<numSprites; i++ )
      if ( strcmp(filename,sprite[i]->name) == 0 )
         foundSprite = sprite[i];
   return foundSprite;
   }


int
SF_SpriteList::Remove(char *name) {
   for ( int i=0; i<numSprites; i++ )
      if ( strcmp(name,sprite[i]->name) == 0 )
         Remove(i);
   return 0;
   }


int
SF_SpriteList::Remove(int j) {
   if ( j >= numSprites )
      return 0;
   for ( int i=j; i<numSprites-1; i++ )
      sprite[i] = sprite[i+1];
   numSprites--;
   return 0;
   }


int
SF_SpriteList::RemoveAll() {
   numSprites = 0;
   return 0;
   }


int
SF_SpriteList::Delete(char *name) {
   for ( int i=0; i<numSprites; i++ )
      if ( strcmp(name,sprite[i]->name) == 0 ) {
         delete sprite[i];
         Remove(i);
         }
   return 0;
   }


int
SF_SpriteList::Delete(int j) {
   if ( j >= numSprites )
      return 0;
   delete sprite[j];
   for ( int i=j; i<numSprites-1; i++ )
      sprite[i] = sprite[i+1];
   numSprites--;
   return 0;
   }


int
SF_SpriteList::DeleteAll() {
   for ( int i=0; i<numSprites; i++ )
     if ( sprite[i] ) {
        delete sprite[i];
        sprite[i] = 0;
        }
   numSprites = 0;
   return 0;
   }
