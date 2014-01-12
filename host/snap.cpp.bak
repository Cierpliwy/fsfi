#include <iostream>
#include <fstream>
#include <libvirt/libvirt.h>
#include <libvirt/virterror.h>

using namespace std;

int usage() {
    cout << "Usage:" << endl
         << "snap <url> <domain> <file>"<< endl;
    return 1;
}

int error() {
    virErrorPtr err = virGetLastError();
    if (err) {
        cout << "ERROR(" << err->code << "): " << err->message << endl;
        return err->code;
    }
    return 0;
}

int error(const string &msg) {
    cout << "ERROR: " << msg << endl;
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 4) return usage();
    virConnectPtr conn = virConnectOpen(argv[1]);
    if (!conn) return error();

    //Load xml
    ifstream in(argv[3]);
    string xml, tmp;

    if (in.good()) {
        while(getline(in, tmp)) {
            xml += tmp;
        }
    } else return error("Cannot open XML file...");

    auto dom = virDomainLookupByName(conn, argv[2]);
    if (dom) {
        virDomainSnapshotCreateXML(dom, xml.c_str(), 0);
        virDomainFree(dom);
    }

    return virConnectClose(conn);
}

