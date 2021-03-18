struct ControllerPort {
  Node::Port port;
  unique_pointer<Controller> device;

  //port.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  ControllerPort(string name);
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto serialize(serializer&) -> void;

protected:
  const string name;
  friend class Controller;
};

extern ControllerPort controllerPort1;
extern ControllerPort controllerPort2;