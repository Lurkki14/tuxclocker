#include "QColorProvider.hpp"

QColorProvider::QColorProvider(){};

QColor QColorProvider::pick(){
  if(current >= colors.count()){
    current = 0;
  }

  return colors.at(current++);
}