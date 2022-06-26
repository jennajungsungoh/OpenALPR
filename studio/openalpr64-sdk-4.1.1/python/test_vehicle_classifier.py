from vehicleclassifier import VehicleClassifier
from argparse import ArgumentParser
import json

parser = ArgumentParser(description='Vehicle Classifier Python Test Program')


parser.add_argument("--config", dest="config", action="store", default="/etc/openalpr/openalpr.conf",
                  help="Path to openalpr.conf config file" )

parser.add_argument("--runtime_data", dest="runtime_data", action="store", default="/usr/share/openalpr/runtime_data",
                  help="Path to OpenALPR runtime_data directory" )


parser.add_argument("--country", dest="country", action="store", default="us",
                  help="Country to use for recognition (default=us") 

parser.add_argument('plate_image', help='Vehicle image file')

options = parser.parse_args()

vehicle_classifier = None
try:
    vehicle_classifier = VehicleClassifier(options.config, options.runtime_data)

    if not vehicle_classifier.is_loaded():
        print("Error loading OpenALPR Vehicle Classifier")
    else:
        print("Using OpenALPR VehicleClassifier: " + vehicle_classifier.get_version())

        vehicle_classifier.set_top_n(7)

        jpeg_bytes = open(options.plate_image, "rb").read()
        results = vehicle_classifier.recognize_array(options.country, jpeg_bytes)

        print json.dumps(results, indent=2)
        # Uncomment to see the full results structure
        # import pprint
        # pprint.pprint(results)

        # print("Image size: %dx%d" %(results['img_width'], results['img_height']))
        # print("Processing Time: %f" % results['processing_time_ms'])
        #
        # i = 0
        # for plate in results['results']:
        #     i += 1
        #     print("Plate #%d" % i)
        #     print("   %12s %12s" % ("Plate", "Confidence"))
        #     for candidate in plate['candidates']:
        #         prefix = "-"
        #         if candidate['matches_template']:
        #             prefix = "*"
        #
        #         print("  %s %12s%12f" % (prefix, candidate['plate'], candidate['confidence']))



finally:
    if vehicle_classifier:
        vehicle_classifier.unload()
