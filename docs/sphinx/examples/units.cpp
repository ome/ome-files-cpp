/*
 * #%L
 * OME-FILES C++ library for image IO.
 * Copyright © 2016 Open Microscopy Environment:
 *   - Massachusetts Institute of Technology
 *   - National Institutes of Health
 *   - University of Dundee
 *   - Board of Regents of the University of Wisconsin-Madison
 *   - Glencoe Software, Inc.
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

#include <iostream>

#include <boost/units/io.hpp>
#include <ome/common/units/length.h>

#include <ome/xml/model/enums/UnitsLength.h>
#include <ome/xml/model/primitives/Quantity.h>

using namespace ome::common::units;
using ome::xml::model::primitives::convert;
using ome::xml::model::primitives::Quantity;
using ome::xml::model::enums::UnitsLength;

namespace
{

  void
  basicUnits()
  {
    std::cout << "Basic unit usage\n";

    /* basic-example-start */
    // Micrometre units.
    micrometre_quantity a(micrometre_quantity::from_value(50.0));
    micrometre_quantity b(micrometre_quantity::from_value(25.3));
    std::cout << "a=" << a << '\n';
    std::cout << "b=" << b << '\n';

    // Arithmetic operations.
    micrometre_quantity c(a + b);
    micrometre_quantity d(a * 8.0);
    micrometre_quantity e(a / 4.0);
    std::cout << "c=" << c << '\n';
    std::cout << "d=" << d << '\n';
    std::cout << "e=" << e << '\n';

    // Unit conversion to SI and Imperial units.
    nanometre_quantity f(c);
    thou_quantity g(c);
    inch_quantity h(c);
    std::cout << "f=" << f << '\n';
    std::cout << "g=" << g << '\n';
    std::cout << "h=" << h << '\n';

    // Unit sytems which do not permit interconversion.
    pixel_quantity i(pixel_quantity::from_value(34.8));
    reference_frame_quantity j(reference_frame_quantity::from_value(2.922));
    std::cout << "i=" << i << '\n';
    std::cout << "j=" << j << '\n';

    // Compilation will fail if uncommented since conversion is
    // impossible.
    // micrometre_quantity k(i);
    // micrometre_quantity l(j);
    /* basic-example-end */
  }

  void
  modelUnits()
  {
    std::cout << "Model unit usage\n";

    /* model-example-start */
    typedef Quantity<UnitsLength, double> Length;

    // Micrometre units.
    Length a(50.0, UnitsLength::MICROMETER);
    Length b(25.3, UnitsLength::MICROMETER);
    std::cout << "a=" << a << '\n';
    std::cout << "b=" << b << '\n';

    // Arithmetic operations.
    Length c(a + b);
    Length d(a * 8.0);
    Length e(a / 4.0);
    std::cout << "c=" << c << '\n';
    std::cout << "d=" << d << '\n';
    std::cout << "e=" << e << '\n';

    // Unit conversion to SI and Imperial units.
    Length f(convert(c, UnitsLength::NANOMETER));
    Length g(convert(c, UnitsLength::THOU));
    Length h(convert(c, UnitsLength::INCH));
    std::cout << "f=" << f << '\n';
    std::cout << "g=" << g << '\n';
    std::cout << "h=" << h << '\n';

    // Unit sytems which do not permit interconversion.
    Length i(34.8, UnitsLength::PIXEL);
    Length j(2.922, UnitsLength::REFERENCEFRAME);
    std::cout << "i=" << i << '\n';
    std::cout << "j=" << j << '\n';

    // Will throw since conversion is impossible.
    try
      {
        Length k(convert(i, UnitsLength::MICROMETER));
      }
    catch (const std::logic_error&)
      {
        std::cout << "i not convertible to µm\n";
      }
    try
      {
        Length l(convert(j, UnitsLength::MICROMETER));
      }
    catch (const std::logic_error&)
      {
        std::cout << "j not convertible to µm\n";
      }
    /* model-example-end */
  }
}

int
main()
{
  basicUnits();
  modelUnits();
}
