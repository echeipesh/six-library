/* =========================================================================
 * This file is part of six-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2019, MDA Information Systems LLC
 *
 * six-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */
#include <string.h>
#include <vector>
#include <iostream>
#include <thread>

#include <std/filesystem>
#include <nitf/coda-oss.hpp>
#include <import/cli.h>
#include <import/io.h>
#include <cphd/CPHDReader.h>
#include <cphd/CPHDXMLControl.h>

namespace fs = std::filesystem;

template <typename T>
T walkCPHDData(const std::byte *data,
                     size_t size,
                     size_t channel)
{
    const T *castData = reinterpret_cast<const T *>(data);
    T cell;
    for (size_t ii = 0; ii < size; ++ii)
    {
        cell = castData[ii];
    }
    return cell;
}

/*!
 *  This extracts raw XML from a CPHD file using the CPHD module
 */
int main(int argc, char** argv)
{
    try
    {
        cli::ArgumentParser parser;
        parser.setDescription("Pull the data from the CPHD imagery.");
        parser.addArgument("cphd", "Input cphd file", cli::STORE, "cphd",
                           "", 1, 1, true);
        parser.addArgument("schema", "Input CPHD schema", cli::STORE, "schema",
                           "", 1, 1, false)->setDefault("");

        // Parse!
        const std::unique_ptr<cli::Results>
            options(parser.parse(argc, (const char**) argv));
        const std::string inputFile = options->get<std::string>("cphd");
        const std::string schemaFile = options->get<std::string>("schema");

        std::vector<std::string> schemaPathnames;
        if (!schemaFile.empty())
        {
            schemaPathnames.push_back(schemaFile);
        }

        // Reads in CPHD and verifies XML using schema
        cphd::CPHDReader reader(inputFile, std::thread::hardware_concurrency());
        cphd::CPHDXMLControl xmlControl;
        const auto xml = xmlControl.toXMLString(reader.getMetadata(), schemaPathnames, true);
        const auto pvp = reader.getPVPBlock();
        const auto numChannels = reader.getNumChannels();
        const auto size = pvp.getPVPsize(0);
        std::vector<std::byte> pvpData;
        pvp.getPVPdata(0, pvpData);
        auto maxValue = *std::max_element(std::begin(pvpData), std::end(pvpData));

        std::unique_ptr<io::OutputStream>
                os;
        os.reset(new io::StandardOutStream());
        os->write(xml);
        os->writeln(std::to_string(numChannels));
        os->writeln(std::to_string(size));
        os->write("PvpData size: ");
        os->writeln(std::to_string(pvpData.size()));
        os->write("Max PVP value:");
        os->writeln(std::to_string((size_t)maxValue));

        const cphd::Wideband& wideband = reader.getWideband();

        std::unique_ptr<std::byte[]> cphdData;
        size_t numThreads = 4;

        size_t data_cell_count = 0;
        for (size_t ii = 0; ii < numChannels; ++ii)
        {
            const types::RowCol<size_t> dims(
                    reader.getMetadata().data.getNumVectors(ii),
                    reader.getMetadata().data.getNumSamples(ii));
            cphdData = wideband.read(ii,
                        0, cphd::Wideband::ALL, 0,
                        cphd::Wideband::ALL,
                        numThreads);

            data_cell_count += dims.area();
            switch (reader.getMetadata().data.getSampleType())
            {
            case cphd::SampleType::RE08I_IM08I:
                walkCPHDData<cphd::zint8_t>(
                    cphdData.get(),
                    dims.area(),
                    ii);
                break;
            case cphd::SampleType::RE16I_IM16I:
                walkCPHDData<cphd::zint16_t>(
                    cphdData.get(),
                    dims.area(),
                    ii);
                break;
            case cphd::SampleType::RE32F_IM32F:
                walkCPHDData<cphd::zfloat >(
                    cphdData.get(),
                    dims.area(),
                    ii);
                break;
            }
        }

        os->write("Wideband data cell count: ");
        os->writeln(std::to_string(data_cell_count));
        os->close();
        return 0;
    }
    catch (const except::Exception& e)
    {
        std::cerr << e.getMessage() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        return 1;
    }
}
