public class Dimensions implements Cloneable {
  
  public Dimensions clone() {
    try {
      return (Dimensions)super.clone();
    } catch(Exception e){ 
      return null;
    }
  }
  
  public int _x,_y,_width,_height;
  private int _offsetX, _offsetY;
  Dimensions(int offsetX, int offsetY, int x, int y, int width, int height) {
    _offsetX = offsetX;
    _offsetY = offsetY;
    setX(x);
    setY(y);
    setHeight(height);
    setWidth(width);
  }
  Dimensions(int offsetX, int offsetY) {
    _x = 0;
    _y = 0;
    _width = 0;
    _height = 0;
    _offsetX = offsetX;
    _offsetY = offsetY;
  }
  
  public int x() {
    return _x + _offsetX;
  }
  public int y() {
    return _y + _offsetY;
  }
  public int width() {
    return _width;
  }
  public int height() {
    return _height;
  }
  
  //Map between local and global
  public int mapToLocalX(int x) {
    return x - x();
  }
  public int mapToLocalY(int y) {
    return y - y();
  }
  public int mapToGlobalX(int x) {
    return x + x();
  }
  public int mapToGlobalY(int y) {
    return y + y();
  }
  
  //In global system:
  public int top() {
    return y();
  }
  public int middle() {
    return y() + height()/2;
  }
  public int bottom() {
    return y()+height();
  }
  public int left() {
    return x();
  }
  public int centre() {
    return x() + width()/2;
  }
  public int right() {
    return x()+width();
  }
  
  
  public void expand(int s) {
    expand(s,s);
  }
  public void expand(int w, int h) {
    _width += (2*w);
    _height += (2*h);
    _x -= w;
    _y -= h;
  }
  public void shift(int x, int y) {
    _x += x;
    _y += y;
  }
  
  public void setOrigin(int x, int y){
    setX(x);
    setY(y);
  }
  public void setSize(int width, int height){
    setHeight(height);
    setWidth(width);
  }
  public void setX(int x){
    _x = x;
  }
  public void setY(int y){
    _y = y;
  }
  public void setWidth(int width){
    _width = width;
  }
  public void setHeight(int height){
    _height = height;
  }
  
}