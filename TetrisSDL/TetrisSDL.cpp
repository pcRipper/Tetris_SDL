#include <ctime>
#include <iostream>
#include <string>
#include <SDL.h>

struct SquareMatrixData {
    uint16_t SizeY;
    uint16_t SizeX;
    SDL_Rect** matrix;
    //
    SDL_Rect* backgroundRect = NULL;
    uint16_t startY;
    uint16_t startX;
    uint16_t topOffset;
    uint16_t leftOffset;
    int backgroundColorRGB;
    int squareColorRGB;
    uint16_t squareSize;
    uint16_t verticalPadding;
    uint16_t horizontalPadding;
    SquareMatrixData(
        uint16_t SizeY, uint16_t SizeX,
        uint16_t startY, uint16_t startX,
        uint16_t topOffset, uint16_t leftOffset,
        int backgroundColorRGB, int squareColorRGB,
        uint16_t squareSize,
        uint16_t verticalPadding, uint16_t horizontalPadding,
        bool setMatrix = true
    ) {
        this->SizeY = SizeY;
        this->SizeX = SizeX;
        this->startY = startY;
        this->startX = startX;
        this->topOffset = topOffset;
        this->leftOffset = leftOffset;
        this->backgroundColorRGB = backgroundColorRGB;
        this->squareColorRGB = squareColorRGB;
        this->squareSize = squareSize;
        this->verticalPadding = verticalPadding;
        this->horizontalPadding = horizontalPadding;

        this->backgroundRect = new SDL_Rect{ this->startX,this->startY,this->getMatrixFieldSizeX(),this->getMatrixFieldSizeY() };

        if (setMatrix) {
            matrix = new SDL_Rect * [SizeY];
            for (int y = 0, sY = topOffset + startY, x, sX; y < SizeY; y++, sY += squareSize + verticalPadding) {
                matrix[y] = new SDL_Rect[SizeX];
                for (x = 0, sX = leftOffset + startX; x < SizeX; x++, sX += squareSize + horizontalPadding) {
                    matrix[y][x] = SDL_Rect{ sX, sY,squareSize,squareSize };
                }
            }
        }
        else matrix = nullptr;

    }
    int getMatrixFieldSizeY(bool withTopOffset = true) {
        return(((withTopOffset) ? topOffset * 2 : 0) + squareSize * SizeY + (SizeY - 1) * verticalPadding);
    }
    int getMatrixFieldSizeX(bool withLeftOffset = true) {
        return(((withLeftOffset) ? leftOffset * 2 : 0) + squareSize * SizeX + (SizeX - 1) * horizontalPadding);
    }
    ~SquareMatrixData() {
        if (matrix != nullptr) {
            for (int k = 0; k < SizeY; k++) {
                delete[] matrix[k];
            }
            delete[] matrix;
        }
    }
};

struct Window {
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_Surface* windowIcon = NULL;
    SDL_Renderer* renderer = NULL;

    bool initWindow(const char* windowTitle, int sizeX, int sizeY, SDL_Surface* icon);
    bool closeWindow();

    ~Window() {
        window = NULL;
        surface = NULL;
        windowIcon = NULL;
        renderer = NULL;
    }
};

bool Window::initWindow(const char* windowTitle, int sizeX, int sizeY,SDL_Surface *icon)
{
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        success = false;
    }
    else
    {
        this->window = SDL_CreateWindow(windowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sizeX, sizeY, SDL_WINDOW_SHOWN);
        if (this->window == NULL)
        {
            success = false;
        }
        else
        {
            this->surface = SDL_GetWindowSurface(this->window);
            SDL_SetWindowIcon(window, icon);
            windowIcon = icon;
        }
    }

    return success;
}

bool Window::closeWindow() {
    SDL_FreeSurface(this->surface);
    SDL_DestroyRenderer(this->renderer);
    SDL_DestroyWindow(this->window);

    if (SDL_GetError() == NULL)return true;

    return false;
}

SDL_Rect* renderBorder(SDL_Rect* block, int lB, int tB, int rB, int bB) {
    SDL_Rect* rect = new SDL_Rect{
        block->x - lB,
        block->y - tB,
        block->w + lB + rB,
        block->h + tB + bB
    };
    return(rect);
};

class Map {
    unsigned short int SizeY;
    unsigned short int SizeX;
    char** field;//current render
    char** fieldPrev;//previous render (for quick segments change)
public:
    int highestPoint = 0;
    bool isChanged = true;
    //
    int symbolsCount = 0;
    char* symbols = nullptr;
    int* symbolsColors = nullptr;
    //
    SquareMatrixData* mapMatrix = NULL;
    //
    Window* window = NULL;
    //
    Map(int sizeY, int sizeX) :
        SizeY(sizeY),
        SizeX(sizeX)
    {

        field = new char* [sizeY];
        fieldPrev = new char* [sizeY];
        for (int y = 0, x; y < sizeY; y++) {
            field[y] = new char[sizeX];
            fieldPrev[y] = new char[sizeX];
            for (x = 0; x < sizeX; x++) {
                field[y][x] = ' ';
                fieldPrev[y][x] = ' ';
            }
        }
    }
    int mapSizeY() { return SizeY; }
    int mapSizeX() { return SizeX; }
    int getSymbolNum(char a);
    void renderField(int type);
    int canChange(int posY, int posX, int sizeY, int sizeX, char** arr, int permission);
    int changeMap(int posY, int posX, int sizeY, int sizeX, char** arr, bool type);
    int checkStreak();
};

int Map::getSymbolNum(char a) {
    if (symbolsCount > 0) {
        for (int k = 0; k < symbolsCount; k++)if (a == symbols[k])return(k);
        return -1;
    }
    else return -1;
}

void Map::renderField(int type = 1) {

    static SDL_Rect* rect = nullptr;

    if (this->isChanged) {
        //Re-render background

        if (!type) {
            SDL_FillRect(window->surface, this->mapMatrix->backgroundRect, this->mapMatrix->backgroundColorRGB);
        }
        for (int y = 0, x, symbol; y < this->SizeY; y++) {
            for (x = 0; x < this->SizeX; x++) {
                symbol = getSymbolNum(field[y][x]);

                if (symbol < 0) {
                    rect = renderBorder(&this->mapMatrix->matrix[y][x],
                        ((x > 0 && this->field[y][x - 1] != ' ') ? 0 : this->mapMatrix->horizontalPadding),
                        ((y > 0 && this->field[y - 1][x] != ' ') ? 0 : this->mapMatrix->verticalPadding),
                        ((x + 1 < this->SizeX && this->field[y][x + 1] != ' ') ? 0 : this->mapMatrix->horizontalPadding),
                        ((y + 1 < this->SizeY && this->field[y + 1][x] != ' ') ? 0 : this->mapMatrix->verticalPadding)
                    );
                }
                else rect = renderBorder(&this->mapMatrix->matrix[y][x], this->mapMatrix->horizontalPadding, this->mapMatrix->verticalPadding, this->mapMatrix->horizontalPadding, this->mapMatrix->verticalPadding);

                if (type) {
                    if (field[y][x] != fieldPrev[y][x]) {
                        if (field[y][x] == ' ') {
                            SDL_FillRect(this->window->surface, rect, this->mapMatrix->squareColorRGB);
                        }
                        else {
                            SDL_FillRect(this->window->surface, rect, this->mapMatrix->backgroundColorRGB);

                            SDL_FillRect(this->window->surface, &this->mapMatrix->matrix[y][x], this->symbolsColors[symbol]);
                        }
                    }
                }
                else {
                    if (symbol > -1) {
                        SDL_FillRect(this->window->surface, rect, this->mapMatrix->backgroundColorRGB);
                    }

                    SDL_FillRect(this->window->surface, ((symbol > -1) ? &this->mapMatrix->matrix[y][x] : rect), ((symbol > -1) ?
                        this->symbolsColors[symbol] :
                        this->mapMatrix->squareColorRGB
                        ));
                }

                delete rect;

            }
        }
        this->isChanged = true;
        SDL_UpdateWindowSurface(window->window);
    }
}

int Map::canChange(int posY, int posX, int sizeY, int sizeX, char** arr, int permission = 0) {
    if (posY + sizeY <= this->SizeY && posX + sizeX <= this->SizeX) {
        for (int y = 0, x; y < sizeY; y++) {
            for (x = 0; x < sizeX; x++)if (arr[y][x] != ' ' && !(this->field[posY + y][posX + x] == ' ' || this->field[posY + y][posX + x] == 'p') && !permission)return 0;
        }
        return 1;
    }
    return 0;
}

int Map::changeMap(int posY, int posX, int sizeY, int sizeX, char** arr = nullptr, bool type = 1) {
    if (!type || this->canChange(posY, posX, sizeY, sizeX, arr)) {
        if (type == 0)for (int y = 0, x; y < this->SizeY; y++)for (x = 0; x < this->SizeX; x++)fieldPrev[y][x] = field[y][x];
        for (int y = 0, x; y < sizeY; y++) {
            for (x = 0; x < sizeX; x++) {
                if (type) {
                    if (arr[y][x] != ' ')this->field[posY + y][posX + x] = arr[y][x];
                }
                else if (type == 0) {
                    if (arr[y][x] != ' ')this->field[posY + y][posX + x] = ' ';
                }
            }
        }
        this->isChanged = true;
        if (type) {
            renderField();
        }
        return(1);
    }
    else return(0);
}

int Map::checkStreak() {
    //declaring lambda nested function for blink animation
    static void (*drawRectLine)(Map obj, int y, int delayTime) = [](Map obj, int y, int delayTime) {
        SDL_FillRects(obj.window->surface, obj.mapMatrix->matrix[y], obj.SizeX, 0xFFFFFF);
        SDL_UpdateWindowSurface(obj.window->window);
        SDL_Delay(delayTime);
    };

    int linesErased = 0;
    for (int y = this->highestPoint, x; y < this->SizeY; y++) {
        for (x = 0; x < this->SizeX; x++)if (this->field[y][x] == ' ')break;
        if (x == this->SizeX) {
            linesErased++;

            //erasing animation start:

            drawRectLine(*this, y, 100);

            renderField(0);//can be optimized to just colorize only 1 erasings line
            SDL_Delay(150);

            drawRectLine(*this, y, 100);

            //end:

            if (y == 0) {
                for (x = 0; x < this->SizeX; x++)field[y][x] = ' ';
            }
            else {
                for (y--; y > -1; y--) {
                    for (x = 0; x < this->SizeX; x++)field[y + 1][x] = field[y][x];
                }
            }
            y = this->highestPoint++;

            renderField(0);

        }
    }
    return(linesErased);
}

/////////////////////

class Blocks {
    class Block {
    public:
        unsigned short int sizeY, sizeX;
        char** arr;
        Block* next;
        Block* nextForm;

        Block(int sizeY, int sizeX, std::string block, Block* nextForm = nullptr) :sizeY(sizeY), sizeX(sizeX) {
            arr = new char* [sizeY];
            for (int k = block.length(), f = 0, y = 0, x = 0; f < k; f++) {
                if (x == 0)arr[y] = new char[sizeX];
                if (block[f] != 'n') {
                    arr[y][x] = block[f];
                    x++;
                }
                else {
                    x = 0;
                    y++;
                }
            }
            next = nullptr;
            this->nextForm = nextForm;
        }

        void addForm(int sizeY, int sizeX, std::string block);
    };
    Block* head = nullptr, * current = nullptr, * currentForm = nullptr;
public:
    Window* window = nullptr;

    SquareMatrixData* blocksMatrix = nullptr;
    /*   Stack<Block> blocksPool;
       int blocksPoolSize = 4;*/
    int blocksPoolSize = 5;
    Block** blocksPool = new Block * [blocksPoolSize];

    Map* BlocksMap = nullptr;
    unsigned int countOfBlocks = 0;

    Block* fallingBlock = nullptr;
    unsigned short int fallingBlockPosY = 0, fallingBlockPosX = 0;



    Block* operator[](int pos) {
        if (head != nullptr) {
            int counter = 0;
            for (current = head; current != nullptr && counter < pos; counter++, current = current->next);
            if (pos == counter && current != nullptr)return current;
        }
        return(nullptr);
    }

    Blocks(Map& obj) :BlocksMap(&obj) {}
    void addBlock(int sizeY, int sizeX, std::string block);
    int pickBlock(Block* obj);
    int moveBlock(int posY, int posX);
    void changeForm();
    void renderBlockStrick(int type);
};

void Blocks::addBlock(int sizeY, int sizeX, std::string block) {
    if (head == nullptr)head = new Block(sizeY, sizeX, block);
    else {
        for (current = head; current->next != nullptr; current = current->next);
        current->next = new Block(sizeY, sizeX, block);
    }
    countOfBlocks++;
}

void Blocks::Block::addForm(int sizeY, int sizeX, std::string block) {
    if (this != nullptr) {
        if (this->nextForm == nullptr)this->nextForm = new Block(sizeY, sizeX, block, this);
        else {
            Block* currentForm;
            for (currentForm = this; currentForm->nextForm != this; currentForm = currentForm->nextForm);
            currentForm->nextForm = new Block(sizeY, sizeX, block, this);
        }
    }
}

int Blocks::pickBlock(Block* obj) {
    if (head != nullptr) {
        fallingBlock = obj;
        fallingBlockPosX = (BlocksMap->mapSizeX() - fallingBlock->sizeX) / 2;
        fallingBlockPosY = 0;
        return 1;
    }
    return 0;
}

int Blocks::moveBlock(int posY, int posX) {
    if (fallingBlock != nullptr) {
        BlocksMap->changeMap(fallingBlockPosY, fallingBlockPosX, fallingBlock->sizeY, fallingBlock->sizeX, fallingBlock->arr, 0);
        if (!BlocksMap->changeMap(posY, posX, fallingBlock->sizeY, fallingBlock->sizeX, fallingBlock->arr)) {
            BlocksMap->changeMap(fallingBlockPosY, fallingBlockPosX, fallingBlock->sizeY, fallingBlock->sizeX, fallingBlock->arr);
            return 0;
        }
        else {
            this->fallingBlockPosY = posY;
            this->fallingBlockPosX = posX;
            return 1;
        }
    }
    else return 0;
}

void Blocks::changeForm() {
    if (fallingBlock->nextForm != nullptr) {
        BlocksMap->changeMap(fallingBlockPosY, fallingBlockPosX, fallingBlock->sizeY, fallingBlock->sizeX, fallingBlock->arr, 0);
        for (int y = 0, x; y <= fallingBlock->nextForm->sizeY; y++) {
            for (x = 0; x <= fallingBlock->nextForm->sizeX; x++) {
                if (BlocksMap->changeMap(fallingBlockPosY - y, fallingBlockPosX - x, fallingBlock->nextForm->sizeY, fallingBlock->nextForm->sizeX, fallingBlock->nextForm->arr)) {
                    fallingBlock = fallingBlock->nextForm;
                    fallingBlockPosY -= y;
                    fallingBlockPosX -= x;
                    return;
                }
            }
        }
        BlocksMap->changeMap(fallingBlockPosY, fallingBlockPosX, fallingBlock->sizeY, fallingBlock->sizeX, fallingBlock->arr);
    }
}

void Blocks::renderBlockStrick(int type = 0) {
    static SDL_Rect* rect = nullptr;

    if (!type) {
        SDL_FillRect(this->window->surface, this->blocksMatrix->backgroundRect, this->blocksMatrix->backgroundColorRGB);
        rect = new SDL_Rect{
            this->blocksMatrix->matrix[0][0].x,this->blocksMatrix->matrix[0][0].y,
            this->blocksMatrix->getMatrixFieldSizeX(false),this->blocksMatrix->getMatrixFieldSizeY(false)
        };
        SDL_FillRect(this->window->surface, rect, this->blocksMatrix->squareColorRGB);
        delete rect;
    }

    float sX = this->blocksMatrix->matrix[0][0].x, sXI, sY;
    int verticalOffset = this->blocksMatrix->SizeY / (this->blocksPoolSize - 1);

    for (int k = 0, symbol; k < this->blocksPoolSize - 1; k++) {

        sXI = this->blocksMatrix->squareSize * (this->blocksMatrix->SizeX - this->blocksPool[k]->sizeX) / 2.0 + sX;
        sY = this->blocksMatrix->matrix[verticalOffset * k][0].y + ((this->blocksMatrix->SizeY - verticalOffset) - this->blocksPool[k]->sizeY) / 2.0;

        for (int y = 0, x; y < this->blocksPool[k]->sizeY; y++) {
            for (x = 0; x < this->blocksPool[k]->sizeX; x++) {

                symbol = this->BlocksMap->symbolsColors[this->BlocksMap->getSymbolNum(this->blocksPool[k]->arr[y][x])];

                if (this->blocksPool[k]->arr[y][x] != ' ') {
                    rect = new SDL_Rect{
                        static_cast<int>(sXI),static_cast<int>(sY),
                        this->blocksMatrix->squareSize,this->blocksMatrix->squareSize
                    };
                    SDL_FillRect(this->window->surface, rect, symbol);
                    delete rect;
                }
                sXI += this->blocksMatrix->squareSize + this->blocksMatrix->verticalPadding;
            }
            sXI = this->blocksMatrix->squareSize * (this->blocksMatrix->SizeX - this->blocksPool[k]->sizeX) / 2.0 + sX;
            sY += this->blocksMatrix->squareSize + this->blocksMatrix->horizontalPadding;
        }

    }

    SDL_UpdateWindowSurface(this->window->window);
}

class Game {
    bool digitNums[11][13] = {
        {0,0,0, 0,0, 0,0,0, 0,0, 0,0,0},//void
        {1,1,1, 1,1, 1,0,1, 1,1, 1,1,1},//0
        {0,0,1, 0,1, 0,0,1, 0,1, 0,0,1},//1
        {1,1,1, 0,1, 1,1,1, 1,0, 1,1,1},//2
        {1,1,1, 0,1, 1,1,1, 0,1, 1,1,1},//3
        {1,0,1, 1,1, 1,1,1, 0,1, 0,0,1},//4
        {1,1,1, 1,0, 1,1,1, 0,1, 1,1,1},//5
        {1,1,1, 1,0, 1,1,1, 1,1, 1,1,1},//6
        {1,1,1, 0,1, 0,0,1, 0,1, 0,0,1},//7
        {1,1,1, 1,1, 1,1,1, 1,1, 1,1,1},//8
        {1,1,1, 1,1, 1,1,1, 0,1, 1,1,1},//9
    };
    Blocks* GameBlocks = nullptr;
    Map* GameMap = nullptr;
    //score and proggressions
    double refreshIntevalMS = 0;
    int score = 0, scoreProgressionLevels, * scoreTrigger = nullptr;
    double* scoreProgression;
    //render score
    SDL_Rect*** numMatrix;
    SDL_Rect* background;
    SquareMatrixData* numMatrixData;
    char* scoreNumbers = new char[9]{ '/','/','/','/','/','/','/' };
public:
    Window* window = NULL;

    Game(Blocks& obj1, Map& obj2, SquareMatrixData& obj3, int scoreProgressionLevels = 0, double* scoreProgression = nullptr, int* scoreTrigger = nullptr) :
        GameBlocks(&obj1), GameMap(&obj2), numMatrixData(&obj3),
        scoreProgressionLevels(scoreProgressionLevels), scoreProgression(scoreProgression)
    {

        if (this->scoreProgression == nullptr || this->scoreProgressionLevels < 1 || scoreTrigger == nullptr) {
            this->scoreProgressionLevels = 7;
            this->scoreProgression = new double[7]{ 300,250,200,150,120,100,80 };
            this->scoreTrigger = new int[6]{ 3000,5000,15000,40000,80000,120000 };
        }

        //

        background = new SDL_Rect{ numMatrixData->startX, numMatrixData->startY,numMatrixData->SizeX, numMatrixData->SizeY };

        int betweenNumsPadding = 6;
        //creating numMatrix
        numMatrix = new SDL_Rect * *[9];
        for (int nums = 0, y, x, maxX, leftOffset = numMatrixData->leftOffset + numMatrixData->startX, numLeftOffset, topOffset; nums < 9; nums++) {

            topOffset = numMatrixData->startY + numMatrixData->topOffset;
            numMatrix[nums] = new SDL_Rect * [5];

            for (y = 0; y < 5; y++, topOffset += numMatrixData->squareSize + numMatrixData->verticalPadding) {

                maxX = ((y + 1) % 2) + 2;
                numLeftOffset = leftOffset;
                numMatrix[nums][y] = new SDL_Rect[maxX];

                for (x = 0; x < maxX; x++, numLeftOffset += numMatrixData->horizontalPadding + numMatrixData->squareSize) {
                    numMatrix[nums][y][x] = SDL_Rect{ numLeftOffset, topOffset,numMatrixData->squareSize,numMatrixData->squareSize };
                    if (maxX == 2 && x == 0)numLeftOffset += numMatrixData->horizontalPadding + numMatrixData->squareSize;
                }
            }

            leftOffset = numLeftOffset + betweenNumsPadding;
        }
    }
    void renderNums(int type);
    void startGame();
};

void Game::renderNums(int type = 1) {
    static int intRanks[3]{
        0x14C814,
        0xC81414,
        0xC814C8
    };

    if (type) {
        std::string currentScore = std::to_string(this->score);
        std::reverse(currentScore.begin(), currentScore.end());
        for (int k = 0, y, x, maxX, digit; k < currentScore.length(); k++) {

            int currentInt = scoreNumbers[k] - 47, drawingInt = currentScore[k] - 47;
            if (currentScore[k] != scoreNumbers[k]) {
                for (y = 0, digit = 0; y < 5; y++) {
                    maxX = ((y + 1) % 2) + 2;
                    for (x = 0; x < maxX; x++, digit++) {
                        if (digitNums[drawingInt][digit] != digitNums[currentInt][digit]) {
                            if (this->digitNums[drawingInt][digit]) {
                                SDL_FillRect(window->surface, &numMatrix[8 - k][y][x], intRanks[k / 3]);//11 constant in class::Game constructor
                            }
                            else {
                                SDL_FillRect(window->surface, &numMatrix[8 - k][y][x], numMatrixData->squareColorRGB);
                            }
                        }
                    }
                }
                scoreNumbers[k] = currentScore[k];
            }
        }
    }
    else {
        SDL_FillRect(this->window->surface, background, numMatrixData->backgroundColorRGB);

        for (int nums = 0, y, x, maxX; nums < 9; nums++) {
            for (y = 0; y < 5; y++) {
                maxX = ((y + 1) % 2) + 2;
                for (x = 0; x < maxX; x++) {
                    SDL_FillRect(window->surface, &numMatrix[nums][y][x], numMatrixData->squareColorRGB);
                }
            }
        }
    }

    SDL_UpdateWindowSurface(this->window->window);

}

void Game::startGame() {
    //
    int blockNum, prevBlock = 0, prevY, prevX, currentLevel = 0;
    bool isSeted;
    double time;
    SDL_bool run = SDL_TRUE;
    SDL_Event eTarget;
    //
    GameMap->highestPoint = GameMap->mapSizeY();
    //
    this->refreshIntevalMS = scoreProgression[currentLevel];

    GameMap->renderField(0);
    renderNums(0);

    //creating pool of next blocks(for test k = 0,realese = 1)
    for (int k = 1; k < GameBlocks->blocksPoolSize; k++) {
        do {
            blockNum = (rand() % GameBlocks->countOfBlocks);
        } while (prevBlock == blockNum);
        prevBlock = blockNum;
        GameBlocks->blocksPool[k] = (*GameBlocks)[blockNum];
    }
    //

    for (; run;) {
    start:
        if (currentLevel < scoreProgressionLevels&& scoreTrigger[currentLevel] <= score) {
            currentLevel++;
            this->refreshIntevalMS = scoreProgression[currentLevel];
        }

        isSeted = false;

        do {
            blockNum = (rand() % GameBlocks->countOfBlocks);
        } while (prevBlock == blockNum);
        prevBlock = blockNum;
        GameBlocks->blocksPool[0] = (*GameBlocks)[blockNum];

        GameBlocks->renderBlockStrick();

        GameBlocks->pickBlock(GameBlocks->blocksPool[GameBlocks->blocksPoolSize - 1]);

        //shift blocksPool to right;
        for (int k = GameBlocks->blocksPoolSize - 1; k > 0; --k) {
            GameBlocks->blocksPool[k] = GameBlocks->blocksPool[k - 1];
        }

        if (GameMap->changeMap(GameBlocks->fallingBlockPosY, GameBlocks->fallingBlockPosX, GameBlocks->fallingBlock->sizeY, GameBlocks->fallingBlock->sizeX, GameBlocks->fallingBlock->arr)) {
            for (time = clock(); run;) {

                if (this->refreshIntevalMS <= (clock() - time)) {
                    if (isSeted) {
                        if (GameMap->highestPoint > GameBlocks->fallingBlockPosY)GameMap->highestPoint = GameBlocks->fallingBlockPosY;

                        this->score += 16 * (currentLevel / 2 + 1);
                        this->score += GameMap->checkStreak() * 160 * (currentLevel / 2 + 1);

                        renderNums(1);
                        GameMap->renderField(1);
                        goto start;
                    }
                    prevY = GameBlocks->fallingBlockPosY;
                    prevX = GameBlocks->fallingBlockPosX;

                    if (!GameBlocks->moveBlock(prevY + 1, prevX)) {
                        isSeted = true;
                    }

                    time = clock();
                }

                while (SDL_PollEvent(&eTarget)) {
                    if (eTarget.type == SDL_QUIT) {
                        run = SDL_FALSE;
                    }
                    else if (eTarget.type == SDL_KEYDOWN) {
                        switch (eTarget.key.keysym.sym) {

                        case SDLK_UP:
                        case SDLK_w:
                            GameBlocks->changeForm();
                            time += refreshIntevalMS * (1 - currentLevel) / 10;
                            isSeted = false;
                            break;

                        case SDLK_DOWN:
                        case SDLK_s:
                            GameBlocks->moveBlock(GameBlocks->fallingBlockPosY + 1, GameBlocks->fallingBlockPosX);
                            time += refreshIntevalMS * (1 - currentLevel) / 10;
                            isSeted = false;
                            break;

                        case SDLK_LEFT:
                        case SDLK_a:
                            if (0 <= GameBlocks->fallingBlockPosX - 1) {
                                GameBlocks->moveBlock(GameBlocks->fallingBlockPosY, GameBlocks->fallingBlockPosX - 1);
                                time += refreshIntevalMS * (1 - currentLevel) / 10;
                                isSeted = false;
                            }
                            break;

                        case SDLK_RIGHT:
                        case SDLK_d:
                            if (GameBlocks->fallingBlockPosX + 1 + GameBlocks->fallingBlock->sizeX <= GameMap->mapSizeX()) {
                                GameBlocks->moveBlock(GameBlocks->fallingBlockPosY, GameBlocks->fallingBlockPosX + 1);
                                time += refreshIntevalMS * (1 - currentLevel) / 10;
                                isSeted = false;
                            }
                            break;

                        case SDLK_p:
                            SDL_Delay(10000);
                            break;

                        case SDLK_ESCAPE:
                            run = SDL_FALSE;
                            break;
                        }

                    }
                }

            }
        }
        else {
            SDL_Delay(3000);
            run = SDL_FALSE;
        }
    }
}

enum Colors {
    iconGrey = 0x9E9E9E,
    iconBlue = 0x3F51B5,
    iconPurple = 0x9C27B0,
    blockYellow = 0xF0F000,
    blockCayan = 0x00F0F0,
    blockRed = 0xF00000,
    blockGreen = 0x00F000,
    blockPurple= 0xA000F0,
    blockBlue = 0x0000F0,
    blockOrange = 0xF0A000,
    blockWhite = 0xFFFFFF,
    blockRed2 = 0xFF0000
};

int SDL_main(int argc, char* argv[])
{
    //0x9E9E9E - grey
    //0x3F51B5 - blue
    //0x9C27B0 - purple
    srand(time(NULL));
    uint16_t iconPixels[256] = {
        iconBlue,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconBlue,
        iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,
        iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,
        iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,
        iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,
        iconBlue,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconPurple,iconPurple,iconPurple,iconPurple,iconBlue,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconPurple,iconBlue,iconBlue,iconBlue,iconBlue,iconPurple,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,
        iconGrey,iconGrey,iconGrey,iconGrey,iconGrey,iconBlue,iconPurple,iconPurple,iconPurple,iconPurple,iconBlue,iconGrey,iconGrey,iconGrey,iconGrey,iconGrey

    };
    //Creating window
    Window window1;

    //Render matrix data for Map

    SquareMatrixData renderDataMap(
        20, 10,
        0, 0,
        8, 8,
        0x141414, 0xC8C8C8,
        25,
        2, 2
    );

    window1.initWindow("Tetris", renderDataMap.getMatrixFieldSizeX() + 100, renderDataMap.getMatrixFieldSizeY() + 65,SDL_CreateRGBSurfaceFrom(iconPixels,16,16,16,32, 0x0f00, 0x00f0, 0x000f, 0xf000));

    //Creating Map and describing blocks colors
    Map tetrisMap(20, 10);
    tetrisMap.mapMatrix = &renderDataMap;
    tetrisMap.window = &window1;
    tetrisMap.symbolsCount = 9;
    tetrisMap.symbols = new char[tetrisMap.symbolsCount]{ '@','#','0','a','$','8','f','-','p' };

    tetrisMap.symbolsColors = new int[tetrisMap.symbolsCount]{
        blockYellow,
        blockCayan,
        blockRed,
        blockGreen,
        blockPurple,
        blockBlue,
        blockOrange,
        blockWhite,
        blockRed2
    };

    SquareMatrixData blocksPool(
        16, 4,
        0, renderDataMap.getMatrixFieldSizeX(),
        100, 6,
        0xC8C8C8,
        0x141414,
        21,
        1, 1,
        true
    );

    Blocks tetrisBlocks(tetrisMap);
    tetrisBlocks.window = &window1;
    tetrisBlocks.blocksMatrix = &blocksPool;

    tetrisBlocks.addBlock(2, 2, "@@n@@");

    tetrisBlocks.addBlock(4, 1, "#n#n#n#");
    tetrisBlocks[1]->addForm(1, 4, "####");

    tetrisBlocks.addBlock(2, 3, "00 n 00");
    tetrisBlocks[2]->addForm(3, 2, " 0n00n0 ");

    tetrisBlocks.addBlock(2, 3, " aanaa ");
    tetrisBlocks[3]->addForm(3, 2, "a naan a");

    tetrisBlocks.addBlock(2, 3, " $ n$$$");
    tetrisBlocks[4]->addForm(3, 2, "$ n$$n$ ");
    tetrisBlocks[4]->addForm(2, 3, "$$$n $ ");
    tetrisBlocks[4]->addForm(3, 2, " $n$$n $");

    tetrisBlocks.addBlock(3, 2, " 8n 8n88");
    tetrisBlocks[5]->addForm(2, 3, "8  n888");
    tetrisBlocks[5]->addForm(3, 2, "88n8 n8 ");
    tetrisBlocks[5]->addForm(2, 3, "888n  8");

    tetrisBlocks.addBlock(3, 2, "f nf nff");
    tetrisBlocks[6]->addForm(2, 3, "fffnf  ");
    tetrisBlocks[6]->addForm(3, 2, "ffn fn f");
    tetrisBlocks[6]->addForm(2, 3, "  fnfff");

    SquareMatrixData renderDataNums(
        65, renderDataMap.getMatrixFieldSizeX() + 100,
        renderDataMap.getMatrixFieldSizeY(), 0,
        5, 20,
        0x5A5A5A, 0xC8C8C8,
        11,
        0, 0,
        false
    );

    Game User(tetrisBlocks, tetrisMap, renderDataNums);
    User.window = &window1;

    User.startGame();

    window1.closeWindow();

    SDL_Quit();

    return(0);
}