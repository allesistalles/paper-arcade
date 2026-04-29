#include "PacMan.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"
#include <cstdlib>
#include <cstring>

const uint8_t PacMan::MAZE[ROWS][COLS] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,2,0,0,0,0,0,1,1,0,0,0,0,0,2,1},
  {1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1},
  {1,0,1,1,0,1,0,0,0,0,1,0,1,1,0,1},
  {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
  {1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1},
  {1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,1},
  {1,1,0,1,0,1,1,0,0,1,1,0,1,0,1,1},
  {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
  {1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int PacMan::dirDx(Dir d) { return (d==D_RIGHT)?1:(d==D_LEFT)?-1:0; }
int PacMan::dirDy(Dir d) { return (d==D_DOWN)?1:(d==D_UP)?-1:0; }
PacMan::Dir PacMan::reverseDir(Dir d) {
  switch(d){ case D_UP:return D_DOWN; case D_DOWN:return D_UP;
             case D_LEFT:return D_RIGHT; case D_RIGHT:return D_LEFT;
             default:return D_NONE; }
}

bool PacMan::canMove(int x, int y, Dir d) const {
  int nx=x+dirDx(d), ny=y+dirDy(d);
  if(nx<0||ny<0||nx>=COLS||ny>=ROWS) return false;
  return _grid[ny][nx] != 1;
}

void PacMan::resetActors() {
  _pac={1,9,D_RIGHT,0}; _pacNext=D_NONE;
  _ghosts[0]={14,1,D_LEFT,0};
  _ghosts[1]={14,9,D_UP,0};
  _ghosts[2]={7,4,D_RIGHT,0};
}

void PacMan::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft=&tft; _scores=&scores;
  _hiScore=scores.getHighScore("pacman");
  _done=false; _gameOver=false; _dirty=true;
  _score=0; _lives=3; _dotsLeft=0;
  for(int y=0;y<ROWS;y++) for(int x=0;x<COLS;x++){
    _grid[y][x]=MAZE[y][x];
    if(MAZE[y][x]==0||MAZE[y][x]==2) _dotsLeft++;
  }
  resetActors();
  _frightened=false;
  srand(millis()); _lastTick=millis();
}

void PacMan::moveActor(Actor& a) {
  if(a.pix==0){ if(!canMove(a.x,a.y,a.dir)) return; }
  a.pix+=5;
  if(a.pix>=TILE){ a.x+=dirDx(a.dir); a.y+=dirDy(a.dir); a.pix=0; }
}

PacMan::Dir PacMan::ghostDir(const Actor& g) const {
  if(_frightened){
    Dir opts[4]={D_UP,D_DOWN,D_LEFT,D_RIGHT};
    for(int i=0;i<4;i++){int j=rand()%4;Dir t=opts[i];opts[i]=opts[j];opts[j]=t;}
    for(int i=0;i<4;i++){
      if(opts[i]==reverseDir(g.dir))continue;
      if(canMove(g.x,g.y,opts[i]))return opts[i];
    }
    return reverseDir(g.dir);
  }
  Dir best=g.dir; int bestD=0x7fff;
  Dir opts[4]={D_UP,D_DOWN,D_LEFT,D_RIGHT};
  for(int i=0;i<4;i++){
    Dir d=opts[i];
    if(d==reverseDir(g.dir)||!canMove(g.x,g.y,d))continue;
    int nx=g.x+dirDx(d),ny=g.y+dirDy(d);
    int dist=abs(nx-_pac.x)+abs(ny-_pac.y);
    if(dist<bestD){bestD=dist;best=d;}
  }
  if(bestD==0x7fff)return reverseDir(g.dir);
  return best;
}

void PacMan::checkGhosts() {
  for(int i=0;i<3;i++){
    if(_ghosts[i].x==_pac.x&&_ghosts[i].y==_pac.y){
      if(_frightened){
        _score+=200; _ghosts[i]={7,4,D_RIGHT,0};
      } else {
        _lives--;
        if(_lives==0) _gameOver=true;
        else resetActors();
        return;
      }
    }
  }
}

void PacMan::update(const InputEvent& input) {
  if(_done) return;
  if(_gameOver){
    if(input.type==InputEvent::TAP){
      _scores->setHighScore("pacman",_score);
      _hiScore=_scores->getHighScore("pacman");
      _done=true;
    }
    return;
  }

  // D-pad zones at bottom (y=220..320)
  if(input.type==InputEvent::TAP && input.y>220){
    int relY=input.y-220;  // 0..100
    if(relY<40 && input.x>70 && input.x<170) _pacNext=D_UP;
    else if(relY>60 && input.x>70 && input.x<170) _pacNext=D_DOWN;
    else if(input.x<80) _pacNext=D_LEFT;
    else if(input.x>160) _pacNext=D_RIGHT;
  }
  // Also accept swipes anywhere for direction change
  if(input.type==InputEvent::SWIPE_LEFT)  _pacNext=D_LEFT;
  if(input.type==InputEvent::SWIPE_RIGHT) _pacNext=D_RIGHT;
  if(input.type==InputEvent::SWIPE_UP)    _pacNext=D_UP;
  if(input.type==InputEvent::SWIPE_DOWN)  _pacNext=D_DOWN;

  uint32_t now=millis();
  if(now-_lastTick<80) return;
  _lastTick=now;

  if(_pac.pix==0&&_pacNext!=D_NONE&&canMove(_pac.x,_pac.y,_pacNext)){
    _pac.dir=_pacNext; _pacNext=D_NONE;
  }
  moveActor(_pac);

  if(_pac.pix==0){
    uint8_t& cell=_grid[_pac.y][_pac.x];
    if(cell==0){cell=3;_score+=10;_dotsLeft--;}
    if(cell==2){cell=3;_score+=50;_dotsLeft--;
                _frightened=true;_frightUntil=now+6000;}
  }
  if(_frightened&&now>_frightUntil) _frightened=false;

  for(int i=0;i<3;i++){
    if(_ghosts[i].pix==0) _ghosts[i].dir=ghostDir(_ghosts[i]);
    moveActor(_ghosts[i]);
  }
  checkGhosts();
  if(_dotsLeft==0) _gameOver=true;
  _dirty=true;
}

void PacMan::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    bool won = (_dotsLeft == 0);
    drawGameOverOverlay(s, "PAC-MAN", _score, Theme::PACMAN565, won);
    return;
  }

  s.fillRect(0, 22, 240, 298, Theme::BG565);

  // Maze (MAZE_Y = 48, safely below Launcher HUD at y=0..21)
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int px = x * TILE;
      int py = MAZE_Y + y * TILE;
      uint8_t v = _grid[y][x];
      if (v == 1)      s.fillRect(px, py, TILE, TILE, Theme::PACMAN565);
      else if (v == 0) s.fillCircle(px + TILE / 2, py + TILE / 2, 2, Theme::MUTED565);
      else if (v == 2) s.fillCircle(px + TILE / 2, py + TILE / 2, 5, Theme::ACCENT565);
    }
  }

  // Pac-Man
  int px = _pac.x * TILE + dirDx(_pac.dir) * _pac.pix + TILE / 2;
  int py = MAZE_Y + _pac.y * TILE + dirDy(_pac.dir) * _pac.pix + TILE / 2;
  s.fillCircle(px, py, 6, Theme::FLAPPY565);

  // Ghosts
  static const uint16_t GH[3] = { Theme::DANGER565, Theme::G2048565, Theme::TETRIS565 };
  for (int i = 0; i < 3; i++) {
    int gx = _ghosts[i].x * TILE + dirDx(_ghosts[i].dir) * _ghosts[i].pix + TILE / 2;
    int gy = MAZE_Y + _ghosts[i].y * TILE + dirDy(_ghosts[i].dir) * _ghosts[i].pix + TILE / 2;
    s.fillCircle(gx, gy, 6, _frightened ? Theme::ACCENT565 : GH[i]);
  }

  // Lives dots at y=310
  for (int i = 0; i < 3; i++) {
    uint16_t col = (i < (int)_lives) ? Theme::PACMAN565 : Theme::SEP565;
    s.fillCircle(108 + i * 14, 310, 4, col);
  }

  // D-pad arrows at bottom
  s.fillTriangle(120, 226, 112, 238, 128, 238, Theme::SEP565);
  s.fillTriangle(120, 318, 112, 306, 128, 306, Theme::SEP565);
  s.fillTriangle(14,  272, 26,  264, 26,  280, Theme::SEP565);
  s.fillTriangle(226, 272, 214, 264, 214, 280, Theme::SEP565);
}

void PacMan::end() {}
#endif
