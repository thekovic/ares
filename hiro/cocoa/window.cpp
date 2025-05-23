#if defined(Hiro_Window)

@implementation CocoaWindow

-(id) initWith:(hiro::mWindow&)windowReference {
  window = &windowReference;
    
  NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
  if(window->state.resizable) style |= NSWindowStyleMaskResizable;

  if(self = [super initWithContentRect:NSMakeRect(0, 0, 640, 480) styleMask:style backing:NSBackingStoreBuffered defer:YES]) {
    [self setDelegate:self];
    [self setReleasedWhenClosed:NO];
    [self setAcceptsMouseMovedEvents:YES];
    [self setTitle:@""];

    NSBundle* bundle = [NSBundle mainBundle];
    NSDictionary* dictionary = [bundle infoDictionary];
    NSString* applicationName = [dictionary objectForKey:@"CFBundleDisplayName"];
    string hiroName = hiro::Application::state().name ? hiro::Application::state().name : string{"hiro"};
    if(applicationName == nil) applicationName = [NSString stringWithUTF8String:hiroName];
      
    statusBar = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    [statusBar setAlignment:NSTextAlignmentLeft];
    [statusBar setBordered:YES];
    [statusBar setBezeled:YES];
    [statusBar setBezelStyle:NSTextFieldSquareBezel];
    [statusBar setEditable:NO];
    [statusBar setHidden:YES];

    [[self contentView] addSubview:statusBar positioned:NSWindowBelow relativeTo:nil];
  }

  return self;
}

-(BOOL) canBecomeKeyWindow {
  return YES;
}

-(BOOL) canBecomeMainWindow {
  return YES;
}

-(void) windowDidMove:(NSNotification*)notification {
  if(auto p = window->self()) p->moveEvent();
}

-(void) windowDidResize:(NSNotification*)notification {
  if(auto p = window->self()) p->sizeEvent();
}

-(BOOL) windowShouldClose:(id)sender {
  if(window->state.onClose) window->doClose();
  else window->setVisible(false);
  if(window->state.modal && !window->visible()) window->setModal(false);
  return NO;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
  return DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
  auto paths = DropPaths(sender);
  if(!paths) return NO;
  window->doDrop(paths);
  return YES;
}

-(NSMenu*) menuBar {
  return [NSApp mainMenu];
}

-(NSTextField*) statusBar {
  return statusBar;
}

-(void)windowDidEnterFullScreen:(NSNotification *)notification {
  window->state.fullScreen = true;
}

-(void)windowDidExitFullScreen:(NSNotification *)notification {
  window->state.fullScreen = false;
}

@end

namespace hiro {

auto pWindow::construct() -> void {
  cocoaWindow = [[CocoaWindow alloc] initWith:self()];
}

auto pWindow::destruct() -> void {
}

auto pWindow::append(sMenuBar menuBar) -> void {
}

auto pWindow::append(sSizable sizable) -> void {
  sizable->setGeometry(self().geometry().setPosition());
  statusBarReposition();
}

auto pWindow::append(sStatusBar statusBar) -> void {
  statusBar->setEnabled(statusBar->enabled(true));
  statusBar->setFont(statusBar->font(true));
  statusBar->setText(statusBar->text());
  statusBar->setVisible(statusBar->visible(true));
}

auto pWindow::focused() const -> bool {
  return [cocoaWindow isMainWindow] == YES;
}

auto pWindow::frameMargin() const -> Geometry {
  NSRect frame = [cocoaWindow frameRectForContentRect:NSMakeRect(0, 0, 640, 480)];
  return {abs(frame.origin.x), (s32)(frame.size.height - 480), (s32)(frame.size.width - 640), abs(frame.origin.y)};
}

auto pWindow::handle() const -> uintptr_t {
  return (uintptr_t)cocoaWindow;
}

auto pWindow::monitor() const -> u32 {
  //TODO
  return 0;
}

auto pWindow::remove(sMenuBar menuBar) -> void {
}

auto pWindow::remove(sSizable sizable) -> void {
  [[cocoaWindow contentView] setNeedsDisplay:YES];
}

auto pWindow::remove(sStatusBar statusBar) -> void {
  [[cocoaWindow statusBar] setHidden:YES];
}

auto pWindow::setBackgroundColor(Color color) -> void {
  NSView* view = cocoaWindow.contentView;
  view.wantsLayer = YES;
  [view.layer
    setBackgroundColor:[[NSColor
      colorWithCalibratedRed:color.red() / 255.0
      green:color.green() / 255.0
      blue:color.blue() / 255.0
      alpha:color.alpha() / 255.0
    ] CGColor]
  ];
}

auto pWindow::setDismissable(bool dismissable) -> void {
  //todo: not implemented
}

auto pWindow::setDroppable(bool droppable) -> void {
  if(droppable) {
    [cocoaWindow registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
  } else {
    [cocoaWindow unregisterDraggedTypes];
  }
}

auto pWindow::setFocused() -> void {
  [cocoaWindow makeKeyAndOrderFront:nil];
}

auto pWindow::setFullScreen(bool fullScreen) -> void {
  if(fullScreen) {
    windowedGeometry = state().geometry;
    [NSApp setPresentationOptions:NSApplicationPresentationFullScreen];
    [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    [cocoaWindow toggleFullScreen:nil];
    state().geometry = _geometry();
  } else {
    [cocoaWindow toggleFullScreen:nil];
    [cocoaWindow setCollectionBehavior:NSWindowCollectionBehaviorDefault];
    [NSApp setPresentationOptions:NSApplicationPresentationDefault];
    state().geometry = windowedGeometry;
  }
}

auto pWindow::setGeometry(Geometry geometry) -> void {
  lock();

  [cocoaWindow
    setFrame:[cocoaWindow
      frameRectForContentRect:NSMakeRect(
        geometry.x(), Desktop::size().height() - geometry.y() - geometry.height(),
        geometry.width(), geometry.height() + statusBarHeight()
      )
    ]
    display:YES
  ];

  if(auto& sizable = state().sizable) {
    sizable->setGeometry(self().geometry().setPosition());
  }

  statusBarReposition();

  unlock();
}

auto pWindow::setMaximized(bool maximized) -> void {
  //todo
}

auto pWindow::setMaximumSize(Size size) -> void {
  [cocoaWindow setContentMaxSize:NSMakeSize(size.width(), size.height())];
}

auto pWindow::setMinimized(bool minimized) -> void {
  [cocoaWindow setIsMiniaturized:minimized];
}

auto pWindow::setMinimumSize(Size size) -> void {
  [cocoaWindow setContentMinSize:NSMakeSize(size.width(), size.height())];
}

auto pWindow::setModal(bool modal) -> void {
  if(modal == true) {
    [NSApp runModalForWindow:cocoaWindow];
  } else {
    [NSApp stopModal];
    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0.0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:event atStart:true];
  }
}

auto pWindow::setResizable(bool resizable) -> void {
  NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
  if(resizable) style |= NSWindowStyleMaskResizable;
  [cocoaWindow setStyleMask:style];
}

auto pWindow::setTitle(const string& text) -> void {
  [cocoaWindow setTitle:[NSString stringWithUTF8String:text]];
}

auto pWindow::setAssociatedFile(const string& filename) -> void {
  [cocoaWindow setRepresentedFilename:[NSString stringWithUTF8String:filename]];
}

auto pWindow::setVisible(bool visible) -> void {
  if(visible) [cocoaWindow makeKeyAndOrderFront:nil];
  else [cocoaWindow orderOut:nil];
}

auto pWindow::moveEvent() -> void {
  if(!locked() && !self().fullScreen() && self().visible()) {
    NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];
    area.size.height -= statusBarHeight();
    state().geometry.setX(area.origin.x);
    state().geometry.setY(Desktop::size().height() - area.origin.y - area.size.height);
  }

  if(!locked()) self().doMove();
}

auto pWindow::sizeEvent() -> void {
  if(!locked() && !self().fullScreen() && self().visible()) {
    NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];
    area.size.height -= statusBarHeight();
    state().geometry.setWidth(area.size.width);
    state().geometry.setHeight(area.size.height);
  }

  if(auto& sizable = state().sizable) {
    sizable->setGeometry(self().geometry().setPosition());
  }

  statusBarReposition();

  if(!locked()) self().doSize();
}

auto pWindow::statusBarHeight() -> u32 {
  if(auto& statusBar = state().statusBar) {
    if(statusBar->visible()) {
      return pFont::size(statusBar->font(true), " ").height() + 6;
    }
  }
  return 0;
}

auto pWindow::statusBarReposition() -> void {
  NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];
  [[cocoaWindow statusBar] setFrame:NSMakeRect(0, 0, area.size.width, statusBarHeight())];
  [[cocoaWindow contentView] setNeedsDisplay:YES];
}

auto pWindow::_append(mWidget& widget) -> void {
  if(auto pWidget = widget.self()) {
    [pWidget->cocoaView removeFromSuperview];
    [[cocoaWindow contentView] addSubview:pWidget->cocoaView positioned:NSWindowAbove relativeTo:nil];
    pWidget->setGeometry(widget.geometry());
    [[cocoaWindow contentView] setNeedsDisplay:YES];
  }
}

auto pWindow::_geometry() -> Geometry {
  NSRect area = [cocoaWindow contentRectForFrameRect:[cocoaWindow frame]];
  area.size.height -= statusBarHeight();
  return {
    (s32)area.origin.x, (s32)(Monitor::geometry(Monitor::primary()).height() - area.origin.y - area.size.height),
    (s32)area.size.width, (s32)area.size.height
  };
}

}

#endif
