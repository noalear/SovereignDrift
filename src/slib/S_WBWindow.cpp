#include "S_OS.h"
#include "S_WBWindow.h"


int
S_WBWindow::Init()
{
  S_Window::Init();
  return 0;
}


S_WBWindow::S_WBWindow()
{
  Init();
}


int
S_WBWindow::Create( S_Root *iroot,
                    S_Object *iparent,
                    char *iwindowName,
                    int (*ipMsgHandler)(S_Window *),
                    int iconID,
                    int ix, int iy, int icx, int icy,
                    ulong iflFrameFlags,
                    void *ipuser)
{
  S_Window::Create(iroot,iparent,iwindowName,ipMsgHandler,
		   iconID,ix,iy,icx,icy,iflFrameFlags,ipuser);
  return 0;
}


int
S_WBWindow::Destroy()
{
  S_Window::Destroy();
  return 0;
}


S_WBWindow::~S_WBWindow()
{
  Destroy();
}
