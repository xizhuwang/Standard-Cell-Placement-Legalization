#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <iomanip> // 

using namespace std;

//去除字串空白
inline string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

//節點結構
struct Node {           
    string name;         //名稱
    double width;        //寬度
    double height;       //高度
    bool isTerminal;     //是否為終端節點

    Node() : width(0.0), height(0.0), isTerminal(false) {}
    Node(string n, double w, double h, bool term)
        : name(n), width(w), height(h), isTerminal(term) {}
};

//位置結構
struct Position {      
    double x; //X座標
    double y; //Y座標
};

//模組結構
struct Block {     
    string name;        //模組名稱
    double width;       //模組寬度
    double height;      //模組高度
    double x;           //當前X座標
    double y;           //當前Y座標
    double origX;       //原始X座標
    double origY;       //原始Y座標
    bool isFixed;       //是否為固定模組

    Block()
        : width(0.0), height(0.0), x(0.0), y(0.0),
        origX(0.0), origY(0.0), isFixed(false) {}
};

//子行結構
struct SubRow {    
    double xStart;                    //子行起始X座標
    double xEnd;                      //子行結束X座標
    double siteWidth;                 //子行的站點寬度
    int numSites;                     //子行的站點數量
    vector<bool> occupiedSites;       //站點佔用狀態
    vector<Block*> placedBlocks;      //已放置的模組按X排序

    SubRow(double xs, int num, double sw)
        : xStart(xs), xEnd(xs + num * sw), siteWidth(sw), numSites(num), occupiedSites(num, false) {}
    
    //插入模組並保持已放置模組的排序
    void insertBlock(Block* block, int startSite, int sitesNeeded) {
        placedBlocks.push_back(block);
        //按照X排序
        sort(placedBlocks.begin(), placedBlocks.end(), [](const Block* a, const Block* b) {
            return a->x < b->x;
        });
        //標記站點為已佔用
        for(int i = startSite; i < startSite + sitesNeeded; ++i){
            if(i >= 0 && i < numSites){
                occupiedSites[i] = true;
            }
        }
    }
    
    //移除模組
    void removeBlock(Block* block, int startSite, int sitesNeeded) {
        auto it = find(placedBlocks.begin(), placedBlocks.end(), block);
        if (it != placedBlocks.end()) {
            placedBlocks.erase(it);
        }
        //標記為未佔用
        for(int i = startSite; i < startSite + sitesNeeded; ++i){
            if(i >= 0 && i < numSites){
                occupiedSites[i] = false;
            }
        }
    }
    
    //查是否可以從startSite開始放置需要的數
    bool canPlaceAt(int startSite, int sitesNeeded) const { 
        if(startSite + sitesNeeded > numSites){
            return false;
        }
        for(int i = startSite; i < startSite + sitesNeeded; ++i){
            if(occupiedSites[i]){
                return false;
            }
        }
        return true;
    }
};

//ROW結構
struct Row {         
    double yStart;                  //起始Y座標
    double height;                  //高度
    double siteWidth;               //寬度
    double siteSpacing;             //間距
    vector<SubRow> subRows;         //行中的子行

    Row()
        : yStart(0.0), height(0.0), siteWidth(0.0),
        siteSpacing(0.0) {}
};

// 布局結構
struct Placement {            
    unordered_map<string, Block> blocks; //所有模組
    vector<Row> rows;                     //所有行
    double maxX;                          //最大X座標
    double maxY;                          //最大Y座標

    Placement() : maxX(0.0), maxY(0.0) {}
};

//宣告
void parseAuxFile(const string& filename, unordered_map<string, string>& files);
void parseNodesFile(const string& filename, unordered_map<string, Node>& nodes);
void parsePlFile(const string& filename, unordered_map<string, Position>& positions);
void parseSclFile(const string& filename, vector<Row>& rows, double& maxX, double& maxY);
void initialPlacement(Placement& placement);
void optimizePlacement(Placement& placement);
double calculateTotalDisplacement(const Placement& placement, double& maxDisplacement);
void writePlFile(const string& filename, const Placement& placement);
void writeNodesFile(const string& filename, const unordered_map<string, Node>& nodes);
void writeSclFile(const string& filename, const vector<Row>& rows);
void writeAuxFile(const string& filename, const string& outputFilePrefix);
void copyFile(const string& srcFilename, const string& destFilename);

// AUX讀檔
void parseAuxFile(const string& filename, unordered_map<string, string>& files) {
    ifstream infile(filename);
    if (!infile) {
        cerr << "無法打開 .aux 檔案：" << filename << endl;
        exit(1);
    }
    string line;
    while (getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find(" : ");
        if (pos != string::npos) {
            string fileList = line.substr(pos + 3);
            istringstream iss(fileList);
            string file;
            while (iss >> file) {
                if (file.find(".nodes") != string::npos) {
                    files["nodes"] = file;
                }
                else if (file.find(".pl") != string::npos) {
                    files["pl"] = file;
                }
                else if (file.find(".scl") != string::npos) {
                    files["scl"] = file;
                }
                else if (file.find(".nets") != string::npos) {
                    files["nets"] = file;
                }
                else if (file.find(".wts") != string::npos) {
                    files["wts"] = file;
                }
                //其他檔案
            }
        }
    }

    //輸出找到的檔案
    /*cout << "解析 .aux 檔案後找到的檔案:\n";
    for (const auto& kv : files) {
        cout << kv.first << " : " << kv.second << "\n";
    }*/
}

// .nodes讀檔
void parseNodesFile(const string& filename, unordered_map<string, Node>& nodes) {
    ifstream infile(filename);
    if (!infile) {
        cerr << "無法打開 .nodes 檔案：" << filename << endl;
        exit(1);
    }
    string line;
    bool headerSkipped = false;
    int lineCount = 0; // 計數讀取的模組數量
    while (getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (!headerSkipped) {
            if (line.find("UCLA nodes") != string::npos) continue;
            if (line.find("NumNodes") != string::npos) continue;
            if (line.find("NumTerminals") != string::npos) continue;
            headerSkipped = true;
            // 不再跳過當前行處理第一個模組行
        }
        // 修剪行首尾空白字符
        string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        istringstream iss(trimmedLine);
        string name;
        double width, height;
        string terminalStr;
        if (!(iss >> name >> width >> height)) {
            cerr << "警告：無法解析模組行（可能格式不正確）：" << trimmedLine << endl;
            continue;
        }
        Node node;
        node.name = name; // **移除 sanitizeName**
        node.width = width;
        node.height = height;
        node.isTerminal = false;
        if (iss >> terminalStr) {
            if (terminalStr == "terminal" || terminalStr == "fixed") {
                node.isTerminal = true;
                // cout << "模組 " << name << " 被標記為 terminal。" << endl;
            }
            else {
                // 除錯：輸出未識別的標籤
                // cout << "模組 " << name << " 的標籤: " << terminalStr << " 不是 'terminal' 或 'fixed'，忽略。" << endl;
            }
        }
        // 檢查是否已存在相同名稱的模組
        if(nodes.find(node.name) != nodes.end()){
            cerr << "警告：發現重複的模組名稱：" << node.name << "，將覆蓋之前的模組。" << endl;
        }
        nodes[node.name] = node;
        lineCount++;
    }
    // 除錯
    //cout << "解析 .nodes 檔案完成，共讀取模組數量：" << lineCount << endl;
}

// .pl 讀檔
void parsePlFile(const string& filename, unordered_map<string, Position>& positions) {
    ifstream infile(filename);
    if (!infile) {
        cerr << "無法打開 .pl 檔案：" << filename << endl;
        exit(1);
    }
    string line;
    bool headerSkipped = false;
    while (getline(infile, line)) {
        // 除錯：輸出讀取的每一行
        //cout << "讀取 .pl 行: \"" << line << "\"\n";

        if (line.empty() || line[0] == '#') continue;
        if (!headerSkipped) {
            if (line.find("UCLA pl") != string::npos) continue;
            headerSkipped = true;
            // 
            // continue;
        }
        string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        istringstream iss(trimmedLine);
        string name;
        double x, y;
        string direction; // 讀取方向，但不使用
        if (!(iss >> name >> x >> y)) {
            cerr << "警告：無法解析模組位置行（可能格式不正確）：" << trimmedLine << endl;
            continue;
        }

        // 讀取方向（例如 ": N"），但不做處理
        iss >> direction;

        // 直接使用模組名稱，不清理
        // name = sanitizeName(name); // **移除 sanitizeName**

        // 除錯：輸出解析後的模組名稱和座標
        //cout << "解析到模組: \"" << name << "\", X: " << x << ", Y: " << y << "\n";

        positions[name] = { x, y };
    }

    // 除錯：輸出解析後的所有模組
    //cout << "解析完成，總共讀取到 " << positions.size() << " 個模組。\n";
}

// .scl 讀檔
void parseSclFile(const string& filename, vector<Row>& rows, double& maxX, double& maxY) {
    ifstream infile(filename);
    if (!infile) {
        cerr << "無法打開 .scl 檔案：" << filename << endl;
        exit(1);
    }
    string line;
    bool inRow = false;
    Row currentRow;
    maxX = 0.0;
    maxY = 0.0;
    while (getline(infile, line)) {
        size_t commentPos = line.find("#");
        if (commentPos != string::npos) {
            line = line.substr(0, commentPos);
        }
        string trimmedLine = trim(line);
        if (trimmedLine.empty()) continue;

        istringstream iss(trimmedLine);
        string keyword;
        iss >> keyword;
        if (keyword == "CoreRow") {
            inRow = true;
            currentRow = Row();
            string direction;
            iss >> direction;
            // cout << "開始解析 CoreRow，方向：" << direction << endl;
        }
        else if (keyword == "End") {
            if (inRow) {
                for (const auto& subrow : currentRow.subRows) {
                    double xEnd = subrow.xEnd;
                    maxX = max(maxX, xEnd);
                }
                maxY = max(maxY, currentRow.yStart + currentRow.height);
                rows.push_back(currentRow);
                // cout << "完成解析 CoreRow，總子行數：" << currentRow.subRows.size() << endl;
                inRow = false;
            }
        }
        else if (inRow) {
            if (keyword == "Coordinate") {
                size_t pos = trimmedLine.find(":");
                if (pos != string::npos) {
                    string valueStr = trim(trimmedLine.substr(pos + 1));
                    currentRow.yStart = stod(valueStr);
                    // cout << "設置行的起始Y座標：" << currentRow.yStart << endl;
                }
            }
            else if (keyword == "Height") {
                size_t pos = trimmedLine.find(":");
                if (pos != string::npos) {
                    string valueStr = trim(trimmedLine.substr(pos + 1));
                    currentRow.height = stod(valueStr);
                    // cout << "設置行的高度：" << currentRow.height << endl;
                }
            }
            else if (keyword == "Sitewidth") {
                size_t pos = trimmedLine.find(":");
                if (pos != string::npos) {
                    string valueStr = trim(trimmedLine.substr(pos + 1));
                    currentRow.siteWidth = stod(valueStr);
                    // cout << "設置行的站點寬度：" << currentRow.siteWidth << endl;
                }
            }
            else if (keyword == "Sitespacing") {
                size_t pos = trimmedLine.find(":");
                if (pos != string::npos) {
                    string valueStr = trim(trimmedLine.substr(pos + 1));
                    currentRow.siteSpacing = stod(valueStr);
                    // cout << "設置行的站點間距：" << currentRow.siteSpacing << endl;
                }
            }
            else if (keyword == "SubrowOrigin") {
                size_t firstColon = trimmedLine.find(':');
                if (firstColon == string::npos) {
                    cerr << "錯誤：無法解析 SubrowOrigin 行：" << trimmedLine << endl;
                    exit(1);
                }

                string rest = trimmedLine.substr(firstColon + 1);
                rest = trim(rest);

                istringstream restIss(rest);
                double xStart;
                if (!(restIss >> xStart)) {
                    cerr << "錯誤：無法解析 SubrowOrigin 的 xStart：" << trimmedLine << endl;
                    exit(1);
                }

                string numSitesLabel;
                if (!(restIss >> numSitesLabel)) {
                    cerr << "錯誤：無法解析 SubrowOrigin 的 NumSites 標籤：" << trimmedLine << endl;
                    exit(1);
                }

                if (numSitesLabel != "NumSites" && numSitesLabel != "Numsites") {
                    cerr << "錯誤：SubrowOrigin 中缺少 NumSites 標籤：" << trimmedLine << endl;
                    exit(1);
                }

                string colon;
                if (!(restIss >> colon) || colon != ":") {
                    cerr << "錯誤：SubrowOrigin 的 NumSites 標籤後缺少冒號：" << trimmedLine << endl;
                    exit(1);
                }

                int numSites;
                if (!(restIss >> numSites)) {
                    cerr << "錯誤：無法解析 SubrowOrigin 的 NumSites 數值：" << trimmedLine << endl;
                    exit(1);
                }

                currentRow.subRows.emplace_back(xStart, numSites, currentRow.siteWidth);
                // cout << "新增子行，xStart：" << xStart << "，NumSites：" << numSites << endl;
            }
        }
    }
}

// 初始擺放
void initialPlacement(Placement& placement) {
    vector<Block*> movableBlocks; // 收集可移動的模組
    for (auto& kv : placement.blocks) {
        if (!kv.second.isFixed) {
            movableBlocks.push_back(&kv.second);
        }
    }

    // 按照模組的原始位置排序，從上到下、從左到右
    sort(movableBlocks.begin(), movableBlocks.end(), [](Block* a, Block* b) {
        if (fabs(a->origY - b->origY) > 1e-6)
            return a->origY < b->origY;
        return a->origX < b->origX;
    });

    // 遍歷所有可移動模組進行放置
    for (auto& block : movableBlocks) {
        bool placed = false;

        // 按照與原始Y座標的距離，對行進行排序
        vector<size_t> rowIndices(placement.rows.size());
        iota(rowIndices.begin(), rowIndices.end(), 0);

        sort(rowIndices.begin(), rowIndices.end(), [&](size_t a, size_t b) {
            double yDiffA = abs(placement.rows[a].yStart - block->origY);
            double yDiffB = abs(placement.rows[b].yStart - block->origY);
            return yDiffA < yDiffB;
        });

        for (size_t idx : rowIndices) {
            Row& row = placement.rows[idx];

            // 檢查模組高度是否小於等於行高度
            if (block->height > row.height + 1e-6) {
                continue; // 模組太高，無法放入此行
            }

            // 計算模組需要的站點數，向上取整
            int sitesNeeded = ceil(block->width / row.siteWidth);

            // 將子行按照與模組x座標的距離排序
            vector<size_t> subrowIndices(row.subRows.size());
            iota(subrowIndices.begin(), subrowIndices.end(), 0);

            sort(subrowIndices.begin(), subrowIndices.end(), [&](size_t a, size_t b) {
                double xCenterA = (row.subRows[a].xStart + row.subRows[a].xEnd) / 2.0;
                double xCenterB = (row.subRows[b].xStart + row.subRows[b].xEnd) / 2.0;
                double xDiffA = abs(xCenterA - block->origX);
                double xDiffB = abs(xCenterB - block->origX);
                return xDiffA < xDiffB;
            });

            for (size_t subIdx : subrowIndices) {
                SubRow& subrow = row.subRows[subIdx];

                // 遍歷子行的所有站點尋找連續的可用站點
                for(int startSite = 0; startSite <= subrow.numSites - sitesNeeded; ++startSite){
                    if(subrow.canPlaceAt(startSite, sitesNeeded)){
                        // 計算對齊後的 x 座標
                        double alignedX = subrow.xStart + startSite * subrow.siteWidth;

                        // 檢查是否超出子行範圍
                        if (alignedX + block->width > subrow.xEnd + 1e-6) {
                            continue; // 放置後會超出子行範圍，則嘗試下一個位置
                        }

                        // 放置模組
                        block->x = alignedX;
                        block->y = row.yStart;
                        subrow.insertBlock(block, startSite, sitesNeeded); // 更新已放置模組列表和站點佔用狀態

                        placed = true;
                        // 除錯輸出
                        // cout << "模組 " << block->name << " 被放置於行 " << idx << " 的子行 " << subIdx << "，站點起始索引：" << startSite << endl;
                        break; // 模組已放置跳出循環
                    }
                }

                if (placed) {
                    break; // 模組已放置跳出循環
                }
            }

            if (placed) {
                break; // 模組已放置跳出循環
            }
        }

        if (!placed) {
            cerr << "錯誤：無法找到足夠的空間放置模組 " << block->name << endl;
            // 繼續嘗試放置其他模組
            // exit(1);
        }
    }
}

// 二次擺放優化
void optimizePlacement(Placement& placement) {
    bool improvement = true; // 避免無限迴圈
    int maxIterations = 6;    // 最大迭代次數
    int currentIteration = 0;

    while (improvement && currentIteration < maxIterations) {
        improvement = false;
        currentIteration++;

        // 收集可移動的模組
        vector<Block*> movableBlocks;
        for (auto& kv : placement.blocks) {
            if (!kv.second.isFixed) {
                movableBlocks.push_back(&kv.second);
            }
        }
        // 按照模組的當前曼哈頓距離從大到小排序
        sort(movableBlocks.begin(), movableBlocks.end(), [&](Block* a, Block* b) {
            double dispA = abs(a->x - a->origX) + abs(a->y - a->origY);
            double dispB = abs(b->x - b->origX) + abs(b->y - b->origY);
            return dispA > dispB;
        });

        for (auto& block : movableBlocks) {
            // 保存當前位移距離
            double originalDisp = abs(block->x - block->origX) + abs(block->y - block->origY);

            // 尋找最佳位置僅在原始位置周圍的曼哈頓距離內搜尋
            size_t bestRowIdx = placement.rows.size();
            double bestX = block->x;
            double bestY = block->y;
            double bestDisp = originalDisp;

            // 動態計算最大曼哈頓距離
            double maxManhattanDist = originalDisp + 20.0; // 搜尋範圍
            for (size_t rowIdx = 0; rowIdx < placement.rows.size(); ++rowIdx) {
                Row& row = placement.rows[rowIdx];

                // 計算垂直距離
                double verticalDist = abs(row.yStart - block->origY);
                if (verticalDist > maxManhattanDist) {
                    continue; // 超出最大垂直距離
                }

                // 計算模組需要的站點數向上取整
                int sitesNeeded = ceil(block->width / row.siteWidth);

                // 計算允許的水平距離
                double remainingDist = maxManhattanDist - verticalDist;
                if (remainingDist < 0) {
                    continue; // 無法在此行放置
                }

                // 計算水平範圍
                double minX = block->origX - remainingDist;
                double maxXPos = block->origX + remainingDist;

                // 確認模組高度是否適合
                if (block->height > row.height + 1e-6) {
                    continue; // 模組太高無法放入此行
                }

                for (auto& subrow : row.subRows) {
                    // 計算候選站點範圍
                    int minSite = static_cast<int>(floor((minX - subrow.xStart) / subrow.siteWidth + 1e-6));
                    int maxSite = static_cast<int>(floor((maxXPos - subrow.xStart - block->width) / subrow.siteWidth + 1e-6));
                    minSite = max(minSite, 0);
                    maxSite = min(maxSite, subrow.numSites - sitesNeeded);

                    for(int siteIdx = minSite; siteIdx <= maxSite; ++siteIdx){
                        if(subrow.canPlaceAt(siteIdx, sitesNeeded)){
                            // 計算對齊後的 x 座標
                            double candidateX = subrow.xStart + siteIdx * subrow.siteWidth;

                            // 檢查是否超出子行範圍
                            if (candidateX + block->width > subrow.xEnd + 1e-6) {
                                continue; // 放置後會超出子行範圍
                            }

                            // 計算新的曼哈頓距離
                            double newDisp = abs(candidateX - block->origX) + abs(row.yStart - block->origY);
                            // 如果新的距離更小，則記錄下來
                            if (newDisp < bestDisp - 1e-6) { // 使用一個小的閾值避免浮點數誤差
                                bestDisp = newDisp;
                                bestRowIdx = rowIdx;
                                bestX = candidateX;
                                bestY = row.yStart;
                            }
                        }
                    }
                }
            }

            // 如果找到更好的位置則更新模組位置
            if (bestDisp < originalDisp - 1e-6) { // 使用一個小的閾值避免浮點數誤差
                // 找到當前模組所在的行和子行，並從中移除模組
                bool removed = false;
                for (size_t rowIdx = 0; rowIdx < placement.rows.size(); ++rowIdx) {
                    Row& row = placement.rows[rowIdx];
                    if (fabs(row.yStart - block->y) < 1e-6) { // 使用 fabs 比較浮點數
                        for (size_t subIdx = 0; subIdx < row.subRows.size(); ++subIdx) {
                            SubRow& subrow = row.subRows[subIdx];
                            // 計算模組所在的站點
                            int startSite = static_cast<int>(floor((block->x - subrow.xStart) / subrow.siteWidth + 1e-6));
                            int sitesOccupied = ceil(block->width / subrow.siteWidth);
                            // 檢查模組是否在此子行範圍內
                            if (block->x >= subrow.xStart - 1e-6 && block->x + block->width <= subrow.xEnd + 1e-6) {
                                if (startSite >=0 && startSite < subrow.numSites && subrow.occupiedSites[startSite]) { // 確保模組被標記為佔用
                                    subrow.removeBlock(block, startSite, sitesOccupied);
                                    // 除錯輸出
                                    // cout << "模組 " << block->name << " 從行 " << rowIdx << " 的子行 " << subIdx << " 移除。" << endl;
                                    removed = true;
                                    break;
                                }
                            }
                        }
                        if (removed) break;
                    }
                }

                if (!removed) {
                    cerr << "錯誤：模組 " << block->name << " 未能從原位置正確移除。" << endl;
                    continue; // 跳過這個模組，避免錯誤
                }

                // 更新模組位置
                block->x = bestX;
                block->y = bestY;

                // 模組放置到新位置的子行中
                Row& newRow = placement.rows[bestRowIdx];
                bool placed = false;
                for (size_t subIdx = 0; subIdx < newRow.subRows.size(); ++subIdx) {
                    SubRow& subrow = newRow.subRows[subIdx];
                    if (block->x >= subrow.xStart - 1e-6 && block->x + block->width <= subrow.xEnd + 1e-6) {
                        // 計算要放置的索引
                        int startSite = static_cast<int>(floor((block->x - subrow.xStart) / subrow.siteWidth + 1e-6));
                        int sitesNeeded = ceil(block->width / subrow.siteWidth);
                        if(subrow.canPlaceAt(startSite, sitesNeeded)){
                            subrow.insertBlock(block, startSite, sitesNeeded);
                            // 除錯輸出
                            // cout << "模組 " << block->name << " 被放置於行 " << bestRowIdx << " 的子行 " << subIdx << "，站點起始索引：" << startSite << endl;
                            placed = true;
                            break;
                        }
                    }
                }
                if (!placed) {
                    cerr << "錯誤：模組 " << block->name << " 在優化後無法正確放置。" << endl;
                    // 可以選擇重新放置或其他處理方式
                    continue;
                }

                improvement = true;
            }
        }
    }
}

//計算總移動距離
double calculateTotalDisplacement(const Placement& placement, double& maxDisplacement) {
    double totalDisplacement = 0.0;
    maxDisplacement = 0.0;
    for (const auto& kv : placement.blocks) {
        const Block& block = kv.second;
        if (!block.isFixed) {
            double dx = abs(block.x - block.origX);
            double dy = abs(block.y - block.origY);
            double displacement = dx + dy; // 曼哈頓距離
            totalDisplacement += displacement;
            if (displacement > maxDisplacement) {
                maxDisplacement = displacement;
            }
        }
    }
    return totalDisplacement;
}

//輸出 .pl 檔案
void writePlFile(const string& filename, const Placement& placement) {
    ofstream outfile(filename);
    if (!outfile) {
        cerr << "無法寫入 .pl 檔案：" << filename << endl;
        exit(1);
    }
    outfile << "UCLA pl 1.0\n\n";
    outfile << fixed << setprecision(6); // 設置輸出精度
    for (const auto& kv : placement.blocks) {
        const Block& block = kv.second;
        outfile << block.name << " " << block.x << " " << block.y;
        /*if (block.isFixed) {
            outfile << " /FIXED";
        }*/
        outfile << "\n";
    }
}

//輸出 .nodes 檔案
void writeNodesFile(const string& filename, const unordered_map<string, Node>& nodes) {
    ofstream outfile(filename);
    if (!outfile) {
        cerr << "無法寫入 .nodes 檔案：" << filename << endl;
        exit(1);
    }
    outfile << "UCLA nodes 1.0\n";
    outfile << "NumNodes : " << nodes.size() << "\n";
    int numTerminals = 0;
    for (const auto& kv : nodes) {
        if (kv.second.isTerminal) {
            numTerminals++;
        }
    }
    outfile << "NumTerminals : " << numTerminals << "\n\n";
    outfile << fixed << setprecision(4); //設置輸出精度
    for (const auto& kv : nodes) {
        outfile << kv.second.name << " " << kv.second.width << " " << kv.second.height;
        if (kv.second.isTerminal) {
            outfile << " terminal";
        }
        outfile << "\n";
    }
}

//輸出 .scl 檔案
void writeSclFile(const string& filename, const vector<Row>& rows) {
    ofstream outfile(filename);
    if (!outfile) {
        cerr << "無法寫入 .scl 檔案：" << filename << endl;
        exit(1);
    }
    outfile << "UCLA scl 1.0\n";
    outfile << "\nNumRows : " << rows.size() << "\n\n";
    outfile << fixed << setprecision(4); // 設置輸出精度
    for (const auto& row : rows) {
        outfile << "CoreRow Horizontal\n";
        outfile << "  Coordinate     : " << row.yStart << "\n";
        outfile << "  Height         : " << row.height << "\n";
        outfile << "  Sitewidth      : " << row.siteWidth << "\n";
        outfile << "  Sitespacing    : " << row.siteSpacing << "\n";
        outfile << "  Siteorient     : 1\n"; //假設為 1
        outfile << "  Sitesymmetry   : 1\n"; //假設為 1
        for (const auto& subrow : row.subRows) {
            outfile << "  SubrowOrigin   : " << subrow.xStart << "    NumSites : " << subrow.numSites << "\n";
        }
        outfile << "End\n\n";
    }
}

//輸出 .aux 檔案
void writeAuxFile(const string& filename, const string& outputFilePrefix) {
    ofstream outfile(filename);
    if (!outfile) {
        cerr << "無法寫入 .aux 檔案：" << filename << endl;
        exit(1);
    }
    outfile << "RowBasedPlacement : " << outputFilePrefix << ".nodes "
        << outputFilePrefix << ".nets "
        << outputFilePrefix << ".wts "
        << outputFilePrefix << ".pl "
        << outputFilePrefix << ".scl\n";
}

//複製檔案
void copyFile(const string& srcFilename, const string& destFilename) {
    ifstream src(srcFilename, ios::binary);
    if (!src) {
        cerr << "錯誤：無法打開來源檔案：" << srcFilename << endl;
        exit(1);
    }
    ofstream dest(destFilename, ios::binary);
    if (!dest) {
        cerr << "錯誤：無法打開目的檔案：" << destFilename << endl;
        exit(1);
    }
    dest << src.rdbuf();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    //檢查
    if (argc != 3) {
        cerr << "使用方式: " << argv[0] << " <input_file_prefix> <output_file_prefix>" << endl;
        return 1;
    }

    string inputFile = argv[1];  // 第一個引數是輸入檔案前綴
    string outputFile = argv[2]; // 第二個引數是輸出檔案前綴

    //輸出執行命令
    cout << "%> " << argv[0] << " " << argv[1] << " " << argv[2] << endl;

    string auxFile = inputFile + ".aux"; //.aux檔案名稱
    unordered_map<string, string> files;
    parseAuxFile(auxFile, files);

    //檢查檔案是否存在
    if (files.find("nodes") == files.end() ||
        files.find("pl") == files.end() ||
        files.find("scl") == files.end() ||
        files.find("nets") == files.end() ||
        files.find("wts") == files.end()) {
        cerr << "錯誤：.aux 檔案中缺少必要的檔案。\n";
        return 1;
    }

    unordered_map<string, Node> nodes;
    parseNodesFile(files["nodes"], nodes);

    unordered_map<string, Position> positions;
    parsePlFile(files["pl"], positions);

    // 解析後的 positions 映射包含以下模組
    /*cout << "解析後的 positions 映射包含以下模組:\n";
    for (const auto& kv : positions) {
        cout << "模組名稱: \"" << kv.first << "\", 座標: (" << kv.second.x << ", " << kv.second.y << ")\n";
    }*/

    vector<Row> rows;
    double maxX, maxY;
    parseSclFile(files["scl"], rows, maxX, maxY);

    // 初始化佈局
    Placement placement;
    for (const auto& kv : nodes) {
        Block block;
        block.name = kv.first;
        block.width = kv.second.width;
        block.height = kv.second.height;
        block.isFixed = kv.second.isTerminal;
        auto it = positions.find(kv.first);
        if (it != positions.end()) {
            block.origX = it->second.x;
            block.origY = it->second.y;
            block.x = it->second.x;
            block.y = it->second.y;
            // 增加除錯輸出
            //cout << "模組 " << block.name << " 原始位置: (" << block.origX << ", " << block.origY << ")\n";
        }
        else {
            // 如果未找到位置則放置在原點
            block.origX = 0.0;
            block.origY = 0.0;
            block.x = 0.0;
            block.y = 0.0;
            // 增加除錯輸出
            //cout << "模組 " << block.name << " 未找到原始位置，設置為原點。\n";
        }
        placement.blocks[kv.first] = block;
    }
    placement.rows = rows;
    placement.maxX = maxX;
    placement.maxY = maxY;

    // 初始擺放
    initialPlacement(placement);
    // 二次優化
    optimizePlacement(placement);

    // 計算總移動距離和最大移動距離
    double maxDisplacementOpt = 0.0;
    double totalDisplacementOpt = calculateTotalDisplacement(placement, maxDisplacementOpt);

    // 輸出優化後的結果
    cout << fixed << setprecision(4);
    cout << "Total displacement: " << totalDisplacementOpt << endl;
    cout << "Maximum displacement: " << maxDisplacementOpt << endl;

    // 寫入輸出檔案
    writeAuxFile(outputFile + ".aux", outputFile);
    writeNodesFile(outputFile + ".nodes", nodes);
    writePlFile(outputFile + ".pl", placement);
    writeSclFile(outputFile + ".scl", rows);
    // 複製 .nets 和 .wts 檔案
    copyFile(files["nets"], outputFile + ".nets");
    copyFile(files["wts"], outputFile + ".wts");

    // 除錯輸出: 檢查模組位置

    return 0;
}
