#pragma once
struct Call_Frame {
  Call_Frame *parent = nullptr;
  std::string name;
};
