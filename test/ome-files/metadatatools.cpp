/*
 * #%L
 * OME-FILES C++ library for image IO.
 * %%
 * Copyright © 2006 - 2015 Open Microscopy Environment:
 *   - Massachusetts Institute of Technology
 *   - National Institutes of Health
 *   - University of Dundee
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
 * Copyright © 2018 Quantitative Imaging Systems, LLC
 * %%
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of any organization.
 * #L%
 */

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/range/size.hpp>

#include <ome/files/FormatException.h>
#include <ome/files/MetadataTools.h>

#include <ome/test/test.h>
#include <ome/test/io.h>

#include <ome/common/module.h>
#include <ome/files/module.h>

#include <ome/common/xml/Platform.h>
#include <ome/common/xml/dom/Document.h>

#include <ome/xml/Document.h>
#include <ome/xml/version.h>
#include <ome/xml/OMETransformResolver.h>
#include <ome/xml/OMETransformResolver.h>
#include <ome/xml/model/Image.h>
#include <ome/xml/model/Instrument.h>
#include <ome/xml/model/LongAnnotation.h>
#include <ome/xml/model/enums/EnumerationException.h>

using boost::filesystem::path;
using boost::filesystem::directory_iterator;
using ome::files::dimension_size_type;
using ome::files::createID;
using ome::files::createDimensionOrder;
using ome::files::createOMEXMLMetadata;
using ome::files::validateModel;
using ome::files::FormatException;
using ome::xml::model::enums::DimensionOrder;
using namespace ome::xml::model::primitives;

TEST(MetadataToolsTest, CreateID1)
{
  std::string e1(createID("Instrument", 0));
  ASSERT_EQ(std::string("Instrument:0"), e1);

  std::string e2(createID("Instrument", 2));
  ASSERT_EQ(std::string("Instrument:2"), e2);

  std::string i1(createID("Image", 4));
  ASSERT_EQ(std::string("Image:4"), i1);
}

TEST(MetadataToolsTest, CreateID2)
{
  std::string d1(createID("Detector", 0, 0));
  ASSERT_EQ(std::string("Detector:0:0"), d1);

  std::string d2(createID("Detector", 2, 5));
  ASSERT_EQ(std::string("Detector:2:5"), d2);

  std::string i1(createID("Shape", 4, 3));
  ASSERT_EQ(std::string("Shape:4:3"), i1);
}

TEST(MetadataToolsTest, CreateID3)
{
  std::string m1(createID("Mask", 0, 0, 0));
  ASSERT_EQ(std::string("Mask:0:0:0"), m1);

  std::string m2(createID("Mask", 3, 5, 6));
  ASSERT_EQ(std::string("Mask:3:5:6"), m2);

  std::string m3(createID("Mask", 92, 329, 892));
  ASSERT_EQ(std::string("Mask:92:329:892"), m3);
}

TEST(MetadataToolsTest, CreateID4)
{
  std::string u1(createID("Unknown", 0, 0, 0, 0));
  ASSERT_EQ(std::string("Unknown:0:0:0:0"), u1);

  std::string u2(createID("Unknown", 5, 23, 6, 3));
  ASSERT_EQ(std::string("Unknown:5:23:6:3"), u2);

  std::string u3(createID("Unknown", 9, 2, 4, 2));
  ASSERT_EQ(std::string("Unknown:9:2:4:2"), u3);
}

TEST(MetadataToolsTest, CurrentModelVersion)
{
  ASSERT_EQ(std::string(OME_XML_MODEL_VERSION), ome::files::getModelVersion());
}

TEST(MetadataToolsTest, ModelVersionFromString)
{
  std::string xml;
  boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));

  readFile(sample_path / "2012-06/multi-channel-z-series-time-series.ome.xml", xml);
  ASSERT_EQ(std::string("2012-06"), ome::files::getModelVersion(xml));
}

TEST(MetadataToolsTest, ModelVersionFromDocument)
{
  ome::common::xml::Platform xmlplat;

  std::string xml;
  boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));

  readFile(sample_path / "2013-06/multi-channel-z-series-time-series.ome.xml", xml);

  ome::common::xml::dom::Document doc = ome::xml::createDocument(xml);
  ASSERT_TRUE(doc);

  ASSERT_EQ(std::string("2013-06"), ome::files::getModelVersion(doc));
}

TEST(MetadataToolsTest, CreateDimensionOrder)
{
  EXPECT_EQ(DimensionOrder::XYZTC, createDimensionOrder(""));
  EXPECT_EQ(DimensionOrder::XYZTC, createDimensionOrder("XYXYZTCZ"));
  EXPECT_EQ(DimensionOrder::XYCZT, createDimensionOrder("XYC"));
  EXPECT_EQ(DimensionOrder::XYTZC, createDimensionOrder("XYTZ"));

  EXPECT_THROW(createDimensionOrder("CXY"), ome::xml::model::enums::EnumerationException);
  EXPECT_THROW(createDimensionOrder("Y"), ome::xml::model::enums::EnumerationException);
  EXPECT_THROW(createDimensionOrder("YC"), ome::xml::model::enums::EnumerationException);
}

struct ModelState
{
  dimension_size_type sizeC;
  dimension_size_type channelCount;
  dimension_size_type samples[6];
};

struct Corrections
{
  path filename;
  bool initiallyValid;
  bool correctable;
  dimension_size_type imageIndex;
  ModelState before;
  ModelState after;
};

typedef ModelState MS;
typedef Corrections Corr;

const std::vector<Corrections> corrections
  {
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/validchannels.ome"),
      true,
      true,
      0,
      { 1, 1, { 1, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/validchannels.ome"),
      true,
      true,
      1,
      { 4, 4, { 1, 1, 1, 1, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/validchannels.ome"),
      true,
      true,
      2,
      { 3, 1, { 3, 0, 0, 0, 0, 0 } },
      { 3, 1, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/validchannels.ome"),
      true,
      true,
      3,
      { 6, 2, { 3, 0, 0, 0, 0, 0 } },
      { 6, 2, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/validchannels.ome"),
      true,
      true,
      4,
      { 4, 2, { 1, 3, 0, 0, 0, 0 } },
      { 4, 2, { 1, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      0,
      { 1, 1, { 1, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      1,
      { 1, 0, { 0, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      2,
      { 1, 1, { 2, 0, 0, 0, 0, 0 } },
      { 2, 1, { 2, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      3,
      { 4, 1, { 1, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      4,
      { 1, 1, { 0, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      5,
      { 4, 4, { 1, 1, 1, 1, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      6,
      { 4, 3, { 1, 1, 1, 0, 0, 0 } },
      { 3, 3, { 1, 1, 1, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      7,
      { 4, 0, { 0, 0, 0, 0, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      8,
      { 4, 4, { 2, 2, 1, 1, 0, 0 } },
      { 6, 4, { 2, 2, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      9,
      { 7, 4, { 1, 1, 1, 1, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      10,
      { 4, 4, { 0, 1, 0, 1, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      11,
      { 4, 4, { 0, 0, 0, 0, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      12,
      { 3, 1, { 3, 0, 0, 0, 0, 0 } },
      { 3, 1, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      13,
      { 3, 0, { 0, 0, 0, 0, 0, 0 } },
      { 3, 3, { 1, 1, 1, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      14,
      { 3, 1, { 5, 0, 0, 0, 0, 0 } },
      { 5, 1, { 5, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      15,
      { 2, 1, { 3, 0, 0, 0, 0, 0 } },
      { 3, 1, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      16,
      { 3, 1, { 0, 0, 0, 0, 0, 0 } },
      { 3, 1, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      17,
      { 6, 2, { 3, 3, 0, 0, 0, 0 } },
      { 6, 2, { 3, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      18,
      { 6, 1, { 3, 0, 0, 0, 0, 0 } },
      { 3, 1, { 3, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      19,
      { 6, 0, { 0, 0, 0, 0, 0, 0 } },
      { 6, 6, { 1, 1, 1, 1, 1, 1 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      20,
      { 6, 2, { 5, 3, 0, 0, 0, 0 } },
      { 8, 2, { 5, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      21,
      { 9, 2, { 3, 3, 0, 0, 0, 0 } },
      { 6, 2, { 3, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      22,
      { 6, 2, { 3, 0, 0, 0, 0, 0 } },
      { 6, 2, { 3, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      23,
      { 6, 2, { 0, 0, 0, 0, 0, 0 } },
      { 6, 2, { 3, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      24,
      { 4, 2, { 1, 3, 0, 0, 0, 0 } },
      { 4, 2, { 1, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      25,
      { 4, 1, { 1, 0, 0, 0, 0, 0 } },
      { 1, 1, { 1, 0, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      26,
      { 4, 0, { 0, 0, 0, 0, 0, 0 } },
      { 4, 4, { 1, 1, 1, 1, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      27,
      { 4, 2, { 5, 3, 0, 0, 0, 0 } },
      { 8, 2, { 5, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      28,
      { 2, 2, { 1, 3, 0, 0, 0, 0 } },
      { 4, 2, { 1, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      29,
      { 4, 2, { 1, 0, 0, 0, 0, 0 } },
      { 4, 2, { 1, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      30,
      { 4, 2, { 0, 3, 0, 0, 0, 0 } },
      { 4, 2, { 1, 3, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-correctable.ome"),
      false,
      true,
      31,
      { 4, 2, { 0, 0, 0, 0, 0, 0 } },
      { 4, 2, { 2, 2, 0, 0, 0, 0 } }
    },
    {
      path(PROJECT_SOURCE_DIR "/test/ome-files/data/brokenchannels-uncorrectable.ome"),
      false,
      false,
      0,
      { 4, 3, { 1, 0, 0, 0, 0, 0 } },
      { 4, 3, { 1, 0, 0, 0, 0, 0 } }
    }
  };

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const Corrections& params)
{
  return os << params.filename << ": Image #" << params.imageIndex;
}

class CorrectionTest : public ::testing::TestWithParam<Corrections>
{
};

TEST_P(CorrectionTest, ValidateAndCorrectModel)
{
  ome::common::xml::Platform xmlplat;

  const Corrections& current(GetParam());
  const dimension_size_type idx(current.imageIndex);

  ome::common::xml::dom::Document doc = ome::xml::createDocument(current.filename);
  ASSERT_TRUE(doc);

  ASSERT_EQ(std::string("2013-06"), ome::files::getModelVersion(doc));

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta(createOMEXMLMetadata(doc));

  {
    const ModelState& state(current.before);

    EXPECT_EQ(PositiveInteger(state.sizeC), meta->getPixelsSizeC(idx));
    EXPECT_EQ(state.channelCount, meta->getChannelCount(idx));
    for (dimension_size_type s = 0; s < boost::size(state.samples); ++s)
      {
        if (state.samples[s] > 0)
          {
            EXPECT_EQ(PositiveInteger(state.samples[s]),
                      meta->getChannelSamplesPerPixel(idx, s));
          }
      }
  }

  if (current.initiallyValid)
    {
      EXPECT_TRUE(validateModel(*meta, false));
    }
  else
    {
      EXPECT_FALSE(validateModel(*meta, false));
      if (current.correctable)
        {
          EXPECT_NO_THROW(EXPECT_FALSE(validateModel(*meta, true)));
        }
      else
        {
          // Totally broken; end test here.
          EXPECT_THROW(validateModel(*meta, true), FormatException);
          return;
        }
    }
  // Model should now be valid.
  EXPECT_TRUE(validateModel(*meta, false));

  {
    const ModelState& state(current.after);

    EXPECT_EQ(PositiveInteger(state.sizeC), meta->getPixelsSizeC(idx));
    EXPECT_EQ(state.channelCount, meta->getChannelCount(idx));
    for (dimension_size_type s = 0; s < boost::size(state.samples); ++s)
      {
        if (state.samples[s] > 0)
          {
            EXPECT_EQ(PositiveInteger(state.samples[s]),
                      meta->getChannelSamplesPerPixel(idx, s));
          }
      }
  }
}

struct ModelTestParameters
{
  path file;
};

template<class charT, class traits>
inline std::basic_ostream<charT,traits>&
operator<< (std::basic_ostream<charT,traits>& os,
            const ModelTestParameters& p)
{
  return os << p.file;
}

namespace
{

  std::vector<ModelTestParameters>
  find_model_tests()
  {
    std::vector<ModelTestParameters> params;

    ome::xml::OMETransformResolver tr;
    std::set<std::string> versions = tr.schema_versions();

    ome::files::register_module_paths();
    boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));
    if (exists(sample_path) && is_directory(sample_path))
      {
        for (directory_iterator si(sample_path); si != directory_iterator(); ++si)
          {
            if (versions.find(si->path().filename().string()) == versions.end())
              continue; // Not a schema directory with transforms.
            path schemadir(si->path());
            if (exists(schemadir) && is_directory(schemadir))
              {
                for (directory_iterator fi(schemadir); fi != directory_iterator(); ++fi)
                  {
                    ModelTestParameters p;
                    p.file = *fi;

                    // 2008-09/instrument.ome.xml
                    if (schemadir.filename() == path("2008-09") &&
                        p.file.filename() == path("instrument.ome.xml"))
                      continue;
                    // timestampannotation.ome.xml - Contains non-POSIX timestamps.
                    if (p.file.filename() == path("timestampannotation.ome.xml"))
                      continue;
                    // Map Annotation cannot be converted
                    if (p.file.filename() == path("mapannotation.ome.xml"))
                      continue;

                    if (p.file.extension() == path(".ome") ||
                        p.file.extension() == path(".xml"))
                      params.push_back(p);
                  }
              }
          }
      }

    return params;
  }

}

std::vector<ModelTestParameters> model_params(find_model_tests());

class ModelTest : public ::testing::TestWithParam<ModelTestParameters>
{
public:
  void SetUp()
  {
    const ModelTestParameters& params = GetParam();
    std::cout << "Source file " << params.file << '\n';
  }
};

TEST_P(ModelTest, CreateMetadataFromFile)
{
  const ModelTestParameters& params = GetParam();

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;
  ASSERT_NO_THROW(meta = createOMEXMLMetadata(params.file));
}

TEST_P(ModelTest, CreateMetadataFromStream)
{
  const ModelTestParameters& params = GetParam();

  boost::filesystem::ifstream input(params.file);

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;
  ASSERT_NO_THROW(meta = createOMEXMLMetadata(input));
}

TEST_P(ModelTest, CreateMetadataFromString)
{
  const ModelTestParameters& params = GetParam();

  std::string input;
  readFile(params.file, input);

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;
  ASSERT_NO_THROW(meta = createOMEXMLMetadata(input));
}

namespace
{

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata>
  createResolutionMetadata()
  {
    boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));
    sample_path /= "2016-06/spim.ome.xml";

    std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;
    meta = createOMEXMLMetadata(sample_path);

    meta->setPixelsSizeX(8192U, 0);
    meta->setPixelsSizeY(8192U, 0);
    meta->setPixelsSizeZ(12U, 0);
    meta->setPixelsSizeX(4096U, 1);
    meta->setPixelsSizeY(4096U, 1);
    meta->setPixelsSizeZ(8U, 1);
    meta->setPixelsSizeX(2048U, 2);
    meta->setPixelsSizeY(2048U, 2);
    meta->setPixelsSizeZ(4U, 2);
    meta->setPixelsSizeX(2048U, 3);
    meta->setPixelsSizeY(2048U, 3);
    meta->setPixelsSizeZ(2U, 3);

    ome::files::addResolutions(*meta, 0, {{4096U,4096U,12U}, {2048U,2048U,12U}, {1024U,1024U,12U}});
    ome::files::addResolutions(*meta, 2, {{1024U,1024U,4U}, {512U,512,4U}});
    ome::files::addResolutions(*meta, 3, {{1024U,1024U,4U}, {512U,512,4U}, {256U,256U,4U}, {128U,128U,4U}, {64U,64U,4U}});

    return meta;
  }

  const std::string test_long_ns("test.org/longnamespace");
  long test_long_val = 342234208992L;

  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata>
  createLongAnnotation()
  {
    boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));
    sample_path /= "2016-06/spim.ome.xml";

    std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;
    meta = createOMEXMLMetadata(sample_path);

    auto long_idx = meta->getLongAnnotationCount();

    std::string annotation_id = createID("Annotation:Long", long_idx);
    meta->setLongAnnotationID(annotation_id, long_idx);
    meta->setLongAnnotationNamespace(test_long_ns, long_idx);
    meta->setLongAnnotationValue(test_long_val, long_idx);

    meta->setInstrumentAnnotationRef(annotation_id, 0U,
                                     meta->getInstrumentAnnotationRefCount(0U));

    meta->resolveReferences(); // Not resolved automatically.

    return meta;
  }

}

TEST(MetadataTools, GetAnnotation)
{
  auto meta = createLongAnnotation();

  ASSERT_EQ(1U, meta->getInstrumentCount());
  ASSERT_EQ(1U, meta->getLongAnnotationCount());

  auto root = meta->getRoot();
  auto omexmlroot = std::dynamic_pointer_cast<ome::xml::meta::OMEXMLMetadataRoot>(root);
  std::shared_ptr<::ome::xml::model::Instrument> minstrument(omexmlroot->getInstrument(0U));

  auto result = ome::files::getAnnotation<::ome::xml::model::Instrument,
    ::ome::xml::model::LongAnnotation>(minstrument, test_long_ns);
  ASSERT_NE(nullptr, result);
  ASSERT_EQ(test_long_val, result->getValue());
}

TEST(MetadataTools, RemoveAnnotation)
{
  auto meta = createLongAnnotation();

  auto root = meta->getRoot();
  auto omexmlroot = std::dynamic_pointer_cast<ome::xml::meta::OMEXMLMetadataRoot>(root);
  std::shared_ptr<::ome::xml::model::Instrument> minstrument(omexmlroot->getInstrument(0U));

  ASSERT_EQ(1U, meta->getLongAnnotationCount());
  ASSERT_EQ(1U, omexmlroot->getInstrument(0U)->sizeOfLinkedAnnotationList());

  ome::files::removeAnnotation<::ome::xml::model::Instrument,
    ::ome::xml::model::LongAnnotation>(minstrument, test_long_ns);

  ASSERT_EQ(1U, meta->getLongAnnotationCount());
  ASSERT_EQ(0U, omexmlroot->getInstrument(0U)->sizeOfLinkedAnnotationList());

  auto result = ome::files::getAnnotation<::ome::xml::model::Instrument,
    ::ome::xml::model::LongAnnotation>(minstrument, test_long_ns);

  ASSERT_EQ(nullptr, result);
}

TEST(MetadataTools, AddResolutions)
{
  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;

  ASSERT_NO_THROW(meta = createResolutionMetadata());
  ASSERT_EQ(4U, meta->getImageCount());
  ASSERT_EQ(3U, meta->getMapAnnotationCount());

  std::cout << "Resolution annotations:\n" << meta->dumpXML() << '\n';
}
TEST(MetadataTools, AddAllResolutions)
{
  std::shared_ptr<::ome::xml::meta::OMEXMLMetadata> meta;

  ASSERT_NO_THROW(meta = createLongAnnotation());

  ome::files::MetadataList<ome::files::Resolution> resolutions =
    {{
        {{4096U,4096U,12U}, {2048U,2048U,12U}, {1024U,1024U,12U}},
        {},
        {{1024U,1024U,4U}, {512U,512,4U}},
        {{1024U,1024U,4U}, {512U,512,4U}, {256U,256U,4U}, {128U,128U,4U}, {64U,64U,4U}}
    }};
  ome::files::addResolutions(*meta, resolutions);

  ASSERT_EQ(4U, meta->getImageCount());
  ASSERT_EQ(3U, meta->getMapAnnotationCount());

  std::cout << "Resolution annotations:\n" << meta->dumpXML() << '\n';
}

TEST(MetadataTools, GetEmptyResolutions)
{
  boost::filesystem::path sample_path(ome::common::module_runtime_path("ome-xml-sample"));
  sample_path /= "2016-06/multi-channel.ome.xml";

  auto meta = createOMEXMLMetadata(sample_path);

  auto res0 = ome::files::getResolutions(*meta, 0);
  ASSERT_EQ(0U, res0.size());
}

TEST(MetadataTools, GetResolutions)
{
  auto meta = createResolutionMetadata();

  auto res0 = ome::files::getResolutions(*meta, 0);
  ASSERT_EQ(3U, res0.size());
  ASSERT_EQ(ome::files::Resolution({4096U,4096U,12U}), res0[0]);
  ASSERT_EQ(ome::files::Resolution({2048U,2048U,12U}), res0[1]);
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,12U}), res0[2]);

  auto res1 = ome::files::getResolutions(*meta, 1);
  ASSERT_EQ(0U, res1.size());

  auto res2 = ome::files::getResolutions(*meta, 2);
  ASSERT_EQ(2U, res2.size());
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,4U}), res2[0]);
  ASSERT_EQ(ome::files::Resolution({512U,512U,4U}), res2[1]);

  auto res3 = ome::files::getResolutions(*meta, 3);
  ASSERT_EQ(5U, res3.size());
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,4U}), res3[0]);
  ASSERT_EQ(ome::files::Resolution({512U,512U,4U}), res3[1]);
  ASSERT_EQ(ome::files::Resolution({256U,256U,4U}), res3[2]);
  ASSERT_EQ(ome::files::Resolution({128U,128U,4U}), res3[3]);
  ASSERT_EQ(ome::files::Resolution({64U,64U,4U}), res3[4]);
}

TEST(MetadataTools, GetAllResolutions)
{
  auto meta = createResolutionMetadata();

  auto allres = ome::files::getResolutions(*meta);

  ASSERT_EQ(4U, allres.size());
  ASSERT_EQ(3U, allres[0].size());
  ASSERT_EQ(ome::files::Resolution({4096U,4096U,12U}), allres[0][0]);
  ASSERT_EQ(ome::files::Resolution({2048U,2048U,12U}), allres[0][1]);
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,12U}), allres[0][2]);

  ASSERT_EQ(0U, allres[1].size());

  ASSERT_EQ(2U, allres[2].size());
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,4U}), allres[2][0]);
  ASSERT_EQ(ome::files::Resolution({512U,512U,4U}), allres[2][1]);

  ASSERT_EQ(5U, allres[3].size());
  ASSERT_EQ(ome::files::Resolution({1024U,1024U,4U}), allres[3][0]);
  ASSERT_EQ(ome::files::Resolution({512U,512U,4U}), allres[3][1]);
  ASSERT_EQ(ome::files::Resolution({256U,256U,4U}), allres[3][2]);
  ASSERT_EQ(ome::files::Resolution({128U,128U,4U}), allres[3][3]);
  ASSERT_EQ(ome::files::Resolution({64U,64U,4U}), allres[3][4]);
}

TEST(MetadataTools, RemoveResolutions)
{
  auto meta = createResolutionMetadata();

  auto root = meta->getRoot();
  auto omexmlroot = std::dynamic_pointer_cast<ome::xml::meta::OMEXMLMetadataRoot>(root);

  ASSERT_EQ(3U, meta->getMapAnnotationCount());
  ASSERT_EQ(5U, omexmlroot->getImage(2U)->sizeOfLinkedAnnotationList());

  ome::files::removeResolutions(*meta, 2U);

  ASSERT_EQ(2U, meta->getMapAnnotationCount());
  ASSERT_EQ(4U, omexmlroot->getImage(2U)->sizeOfLinkedAnnotationList());
}

TEST(MetadataTools, RemoveAllResolutions)
{
  auto meta = createResolutionMetadata();

  auto root = meta->getRoot();
  auto omexmlroot = std::dynamic_pointer_cast<ome::xml::meta::OMEXMLMetadataRoot>(root);

  ASSERT_EQ(3U, meta->getMapAnnotationCount());
  ASSERT_EQ(5U, omexmlroot->getImage(0U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(4U, omexmlroot->getImage(1U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(5U, omexmlroot->getImage(2U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(5U, omexmlroot->getImage(3U)->sizeOfLinkedAnnotationList());

  ome::files::removeResolutions(*meta);

  ASSERT_EQ(0U, meta->getMapAnnotationCount());
  ASSERT_EQ(4U, omexmlroot->getImage(0U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(4U, omexmlroot->getImage(1U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(4U, omexmlroot->getImage(2U)->sizeOfLinkedAnnotationList());
  ASSERT_EQ(4U, omexmlroot->getImage(3U)->sizeOfLinkedAnnotationList());

  ASSERT_EQ(0U, ome::files::getResolutions(*meta, 0).size());
  ASSERT_EQ(0U, ome::files::getResolutions(*meta, 1).size());
  ASSERT_EQ(0U, ome::files::getResolutions(*meta, 2).size());
  ASSERT_EQ(0U, ome::files::getResolutions(*meta, 3).size());
}

// Disable missing-prototypes warning for INSTANTIATE_TEST_CASE_P;
// this is solely to work around a missing prototype in gtest.
#ifdef __GNUC__
#  if defined __clang__ || defined __APPLE__
#    pragma GCC diagnostic ignored "-Wmissing-prototypes"
#  endif
#  pragma GCC diagnostic ignored "-Wmissing-declarations"
#endif

INSTANTIATE_TEST_CASE_P(CorrectionVariants, CorrectionTest, ::testing::ValuesIn(corrections));
INSTANTIATE_TEST_CASE_P(ModelVariants, ModelTest, ::testing::ValuesIn(model_params));
