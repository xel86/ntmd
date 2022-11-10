
namespace ntmd {

class ArgumentParser
{
  public:
    ArgumentParser(int argc, char** argv);
    ~ArgumentParser();

    /* All arguments and their defaults */
    bool daemon{false};
    bool debug{false};
    int interval{10};
};

} // namespace ntmd