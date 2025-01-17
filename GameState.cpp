/*
* GameState.cpp, 8/8/2020 12:04 PM
*/

struct Matrix4x4 {
    float M[4][4];

    float* operator[] (int index) {
        return M[index];
    }
};

static std::map<string, int> game_state_offsets {
    {"name", 0x10},
};

class GameState : public RemoteMemoryObject {
public:

    wstring name;
    int id;

    GameState(int id, addrtype address, FieldOffsets* offsets = &game_state_offsets)
        : id(id), RemoteMemoryObject(address, offsets)
    {
    }

    bool is(wstring name) {
        return !name.compare(this->name);
    }
};

std::map<string, int> in_game_state_offsets {
    {"name",          0x10},
    {"load_stage1",   0x40},
    {"in_game_ui",    0x80},
    {"in_game_data", 0x500},
    {"server_data",  0x508},
    {"ui_root",      0x630},
    {"hovered",      0x698},
    {"hovered_item", 0x6a8},
    {"time_in_game", 0x6dc},
    {"load_stage2",  0x6f0},
    {"camera",      0x1178},
    {"width",       0x1180},
    {"height",      0x1184},
    {"matrix",      0x1238},
};

class InGameState : public GameState {
public:

    unique_ptr<InGameUI> igu;
    unique_ptr<InGameData> igd;
    unique_ptr<ServerData> sd;
    int load_stage = 0;

    float width, height;
    float center_x, center_y;

    InGameState(int id, addrtype address) : GameState(id, address, &in_game_state_offsets)
    {
        name = L"InGameState";
    }

    InGameUI* in_game_ui() {
        addrtype addr = read<addrtype>("in_game_ui");
        if (!igu || igu->address != addr)
            igu.reset(new InGameUI(addr));

        return igu.get();
    }

    InGameData* in_game_data() {
        addrtype addr = read<addrtype>("in_game_data");
        if (!igd || igd->address != addr)
            igd.reset(new InGameData(addr));

        return igd.get();
    }

    ServerData* server_data() {
        addrtype addr = read<addrtype>("in_game_data", "server_data");
        if (!sd || sd->address != addr)
            sd.reset(new ServerData(addr));

        return sd.get();
    }

    Element* get_hovered_element() {
        addrtype addr = read<addrtype>("hovered");
        return addr ? new Element(addr) : nullptr;
    }

    Item* get_hovered_item() {
        addrtype addr = read<addrtype>("hovered_item", 0x428);
        return addr ? new Item(addr) : nullptr;
    }

    unsigned int time_in_game() {
        return read<unsigned int>("time_in_game");
    }

    void reset() {
        igu.reset();
        igd.reset();
        sd.reset();

        width = read<int>("width");
        height = read<int>("height");
        center_x = width / 2;
        center_y = height / 2;

        Element ui_root(read<addrtype>("ui_root"));
        float root_scale = ui_root.scale();
        float aspect_ratio =  width / 1.6f / height;
        g_scale_x = width / 2560.0f / aspect_ratio / root_scale;
        g_scale_y = height / 1600.0f / root_scale;
    }

    Vector3& transform(Vector3& vec) {
        static Matrix4x4 mat;
        static Point size;
        static unsigned expiration_time;

        if (GetTickCount() > expiration_time) {
            mat = read<Matrix4x4>("matrix");
            size = read<Point>("width");
            expiration_time = GetTickCount() + 30;
        }

        float x = vec.x * mat[0][0] + vec.y * mat[1][0] + vec.z * mat[2][0] + mat[3][0];
        float y = vec.x * mat[0][1] + vec.y * mat[1][1] + vec.z * mat[2][1] + mat[3][1];
        float z = vec.x * mat[0][2] + vec.y * mat[1][2] + vec.z * mat[2][2] + mat[3][2];
        float w = vec.x * mat[0][3] + vec.y * mat[1][3] + vec.z * mat[2][3] + mat[3][3];

        vec.x = (1.0 + x / w) * size.x / 2;
        vec.y = (1.0 - y / w) * size.y / 2;
        vec.z = z / w;

        return vec;
    }
};
