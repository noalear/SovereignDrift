#include "S_OS.h"
#include "S_Tree.h"


int
S_Tree::Init()
{
  name = NULL;
  type = 0;
  level = 0;
  count = 1;
  prev = next = child = parent = NULL;
  return 0;
}


S_Tree::S_Tree(char *iname,int itype)
{
  Init();
  Create(iname,itype);
}


int
S_Tree::Create(char *iname,int itype)
{
  Destroy();
  if ( iname )
    {
      if ( name )
	delete name;
      name = S_strdup(iname);
    }
  type = itype;
  return 0;
}


int
S_Tree::Destroy() {
  if ( child )
    child->parent = NULL;
  Remove();
  if ( name )
    {
      S_free(name);
      name = NULL;
    }
  Init();
  return 0;
}


S_Tree::~S_Tree() {
   Destroy();
   }


int
S_Tree::Remove()
{
  if ( next )
    next->prev = prev;
  if ( prev )
    prev->next = next;
  if ( parent && parent->child == this )
    parent->child = next;
  next = prev = parent = NULL;
  return 0;
}


int
S_Tree::InsertBefore(S_Tree *object)
{
  next = prev = NULL;
  if ( object )
    {
      if ( object->prev )
	{
	  prev = object->prev;
	  prev->next = this;
	}
      next = object;
      object->prev = this;
      parent = object->parent;
      object->level = level;
    }
  return 0;
}


int
S_Tree::InsertAfter(S_Tree *object)
{
  next = prev = NULL;
  if ( object )
    {
      if ( object->next )
	{
	  next = object->next;
	  next->prev = this;
	}
      prev = object;
      object->next = this;
      parent = object->parent;
      object->level = level;
    }
  return 0;
}


int
S_Tree::AddChild(S_Tree *ichild)
{
  if ( !ichild )
    return 0;
  ichild->parent = this;
  ichild->level = level + 1;
  
  if ( !child )
    {
      child = ichild;
      child->next = child->prev = NULL;
    }
  else
    {
      S_Tree *tmp;
      for ( tmp=child; tmp->next!=NULL; tmp=tmp->next );
      ichild->InsertAfter(tmp);
    }
  
  return 0;
}


int
S_Tree::DeleteChildren()
{
  
  for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
    child->DeleteChildren();
  
  while ( child )
    delete child;
  
  return 0;
}


S_Tree *
S_Tree::GetChildNamed(char *iname)
{
  for ( S_Tree *tmp=child; tmp!=NULL; tmp=tmp->next )
    {
      if ( !strcmp(tmp->name,iname) )
	{
	  return tmp;
	}
    }
  return NULL;
}
