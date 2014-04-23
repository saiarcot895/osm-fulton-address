#include "fultoncountyconverter.h"
#include <QStringBuilder>
#include <QDebug>
#include <QUrl>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QUrlQuery>
#endif

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <geos/algorithm/Angle.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/prep/PreparedPolygon.h>
#include <geos/util.h>

FultonCountyConverter::FultonCountyConverter(QObject* parent) :
    QObject(parent),
    nam(new QNetworkAccessManager(parent)),
    factory(new geos::geom::GeometryFactory(new geos::geom::PrecisionModel(), 4326)),
    newline(QLatin1Literal("\n"))
{
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(readOSM(QNetworkReply*)));
}

void FultonCountyConverter::setBoundingBox(double top, double left, double bottom, double right) {
    this->top = top;
    this->left = left;
    this->bottom = bottom;
    this->right = right;
}

void FultonCountyConverter::setAddresses(QString addressesFile) {
    this->addressesFile.setFileName(addressesFile);
}

void FultonCountyConverter::setBuildings(QString buildingsFile) {
    this->buildingsFile.setFileName(buildingsFile);
}

void FultonCountyConverter::setTaxParcels(QString taxParcelsFile) {
    this->taxParcelsFile.setFileName(taxParcelsFile);
}

void FultonCountyConverter::setZipCodes(QString zipCodesFile) {
    this->zipCodesFile.setFileName(zipCodesFile);
}

void FultonCountyConverter::setLogOptions(Logging logOptions) {
    this->logOptions = logOptions;
}

void FultonCountyConverter::convert() {
    if (!newAddresses.isEmpty() || !excludedAddresses.isEmpty() || !buildings.isEmpty() || !addressBuildings.isEmpty()) {
        qWarning() << "Already converted. Doing nothing.";
        return;
    }

    downloadOSM();
}

QString FultonCountyConverter::getLog() {
    return output;
}

void FultonCountyConverter::downloadOSM() {
    QString query = QString("[out:xml];"
            "(node[\"addr:housenumber\"](%1,%2,%3,%4);"
            "way[\"addr:housenumber\"](%1,%2,%3,%4);"
            "way[\"name\"](%1,%2,%3,%4);"
            "way[\"building\"](%1,%2,%3,%4);"
            "relation[\"addr:housenumber\"](%1,%2,%3,%4););"
            "out meta;>;out meta;")
            .arg(bottom - (15.0 / DEGREES_TO_METERS))
            .arg(left - (15.0 / DEGREES_TO_METERS))
            .arg(top + (15.0 / DEGREES_TO_METERS))
            .arg(right + (15.0 / DEGREES_TO_METERS));
    QUrl url("http://overpass-api.de/api/interpreter/");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QUrlQuery queries;
    queries.addQueryItem("data", query);
    url.setQuery(queries);
#else
    url.addQueryItem("data", query);
#endif
    nam->get(QNetworkRequest(url));
}

void FultonCountyConverter::readOSM(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QXmlStreamReader reader(reply->readAll());
        Street street;
        Address address;
        uint id = 0;
        int version = -1;
        QList<uint> nodeIndices;
        QMap<QString, QString> tags;
        QString user;
        uint uid;
        uint changesetID;
        QDateTime timestamp;
        FeatureType current = None;
        while (!reader.atEnd()) {
            // Tags are assumed to be in alphabetical order
            switch (reader.readNext()) {
                case QXmlStreamReader::StartElement:
                    if (reader.name().toString() == "node") {
                        // Store all nodes
                        Node node;
                        geos::geom::Coordinate coordinate;
                        node.id = reader.attributes().value("id").toString().toUInt();
                        node.version = reader.attributes().value("version").toString().toInt();
                        coordinate.y = reader.attributes().value("lat").toString().toDouble();
                        coordinate.x = reader.attributes().value("lon").toString().toDouble();
                        node.point = QSharedPointer<geos::geom::Point>(factory->createPoint(coordinate));
                        nodes.insert(node.id, node);
                    } else if (reader.name().toString() == "way") {
                        // A way is typically either be a road or a building. For
                        // now, assume it is a road, since we need the nodes and
                        // those come before the way tags
                        current = Way;
                        id = reader.attributes().value("id").toUInt();
                        version = reader.attributes().value("version").toInt();
                        user = reader.attributes().value("user").toString();
                        uid = reader.attributes().value("uid").toUInt();
                        changesetID = reader.attributes().value("changeset").toUInt();
                        timestamp = QDateTime::fromString(reader.attributes().value("timestamp").toString(), Qt::ISODate);
                    } else if (reader.name().toString() == "tag") {
                        if (reader.attributes().value("k") == "addr:housenumber") {
                            address = Address();
                            address.setHouseNumber(reader.attributes().value("v").toString());
                        } else if (reader.attributes().value("k") == "addr:street") {
                            // We don't care what street instance the address is
                            // linked to. If there is any address with the same number
                            // and street name, skip it.
                            if (address.street().name().isEmpty()) {
                                address.street().setName(reader.attributes().value("v").toString());
                                existingAddresses.append(address);
                            }
                        } else if (current == Way && reader.attributes().value("k") == "building") {
                            // We now know this is a building.
                            current = BuildingConfirmed;
                        } else if (reader.attributes().value("k") == "highway"
                                && current == Way) {
                            // We now know this is a way.
                            current = WayConfirmed;
                            street = Street();
                            street.setNodeIndices(nodeIndices);
                        } else if (reader.attributes().value("k") == "name"
                                && current == WayConfirmed) {
                            street.setName(reader.attributes().value("v").toString());
                        }
                        tags.insert(reader.attributes().value("k").toString(),
                                    reader.attributes().value("v").toString());
                    } else if (reader.name().toString() == "nd" && current == Way) {
                        nodeIndices.append(reader.attributes().value("ref").toString().toUInt());
                    }
                    break;
                case QXmlStreamReader::EndElement:
                    if (reader.name().toString() == "way") {
                        if (current == WayConfirmed) {
                            streets.insertMulti(street.name().toUpper(), street);
                            current = None;
                        } else if (current == BuildingConfirmed) {
                            Building building;
                            building.setId(id);
                            building.setVersion(version);
                            building.setNodeIndices(nodeIndices);
                            building.setTags(tags);
                            building.setUser(user);
                            building.setUid(uid);
                            building.setChangesetID(changesetID);
                            building.setTimestamp(timestamp);
                            existingBuildings.append(building);
                            current = None;
                        }
                        nodeIndices.clear();
                        tags.clear();
                    }
                    break;
                default:
                    break;
            }
        }

        QList<Street> streetValues = streets.values();
        for (int i = 0; i < streetValues.size(); i++) {
            Street street = streetValues.at(i);
            geos::geom::CoordinateSequence* nodePoints = factory
                    ->getCoordinateSequenceFactory()->create(
                    (std::vector<geos::geom::Coordinate>*) NULL, 2);
            for (int j = 0; j < street.nodeIndices().size(); j++) {
                nodePoints->add(*(nodes.value(street.nodeIndices().at(j)).point->getCoordinate()));
            }
            street.setPath(QSharedPointer<geos::geom::LineString>(factory->createLineString(nodePoints)));
        }

        for (int i = 0; i < existingBuildings.size(); i++) {
            Building building = existingBuildings.at(i);
            geos::geom::CoordinateSequence* nodePoints = factory
                    ->getCoordinateSequenceFactory()->create(
                    (std::vector<geos::geom::Coordinate>*) NULL, 2);
            for (int j = 0; j < building.nodeIndices().size(); j++) {
                geos::geom::Coordinate coord = *(nodes.value(building.nodeIndices().at(j))
                        .point->getCoordinate());
                nodePoints->add(coord);
            }
            try {
                geos::geom::LinearRing* ring = factory->createLinearRing(nodePoints);
                building.setBuilding(QSharedPointer<geos::geom::Polygon>(factory
                        ->createPolygon(ring, NULL)));
                existingBuildings.replace(i, building);

                const geos::geom::Envelope* envelope = building.building()->getEnvelopeInternal();
                double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                buildingsTree.insert(min, max, building);

                delete[] min;
                delete[] max;
            } catch (...) {
                qDebug() << "Building skipped";
                existingBuildings.removeAt(i);
                i--;
            }
        }

        if (logOptions & ExistingStreets) {
            output = output % QStringLiteral("Streets:") % newline;
            QList<Street> streetValues = streets.values();
            for (int i = 0; i < streetValues.size(); i++) {
                Street street = streetValues.at(i);
                output = output % street.name() % newline;
            }
        }
        if (logOptions & ExistingAddresses) {
            output = output % newline;
            output = output % QStringLiteral("Existing Addresses:");
            for (int i = 0; i < existingAddresses.size(); i++) {
                Address address = existingAddresses.at(i);
                output = output % (address.houseNumber() + " " + address.street().name()) % newline;
            }
        }
        readZipCodeFile();
    } else {
        output = output % reply->errorString() % newline;
        qCritical() << reply->errorString();
        cleanup();
    }
}

void FultonCountyConverter::readZipCodeFile() {
    if (!zipCodesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Couldn't open zip codes: " << zipCodesFile.errorString();
        readBuildingFile();
        return;
    }

    QHash<int, geos::geom::Point*> zipCodeNodes;
    QHash<int, geos::geom::LineString*> zipCodeWays;

    QXmlStreamReader reader(&zipCodesFile);
    int wayId;
    int zipCode;
    bool polygon = false;
    geos::geom::CoordinateSequence* baseSequence = NULL;
    QList<geos::geom::Geometry*> holes;
    QList<geos::geom::Coordinate> coordinates;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    geos::geom::Coordinate coordinate;
                    coordinate.y = reader.attributes().value("lat").toString().toDouble();
                    coordinate.x = reader.attributes().value("lon").toString().toDouble();
                    int nodeId = reader.attributes().value("id").toString().toInt();
                    zipCodeNodes.insert(nodeId, factory->createPoint(coordinate));
                } else if (reader.name().toString() == "way") {
                    wayId = reader.attributes().value("id").toString().toInt();
                } else if (reader.name().toString() == "member") {
                    if (reader.attributes().value("role").toString() == "outer") {
                        geos::geom::CoordinateSequence* sequence = zipCodeWays
                                .value(reader.attributes().value("ref").toString().toInt())
                                ->getCoordinates();
                        if (baseSequence == NULL) {
                            baseSequence = sequence;
                        } else {
                            if (sequence->front().equals2D(baseSequence->back())) {
                                baseSequence->add(sequence, true, true);
                                baseSequence->removeRepeatedPoints();
                                delete sequence;
                            } else if (sequence->back().equals2D(baseSequence->front())) {
                                sequence->add(baseSequence, true, true);
                                sequence->removeRepeatedPoints();
                                delete baseSequence;
                                baseSequence = sequence;
                            } else if (sequence->size() > baseSequence->size()) {
                                baseSequence = sequence;
                            }
                        }
                    } else if (reader.attributes().value("role").toString() == "inner") {
                        geos::geom::CoordinateSequence* sequence = zipCodeWays
                                .value(reader.attributes().value("ref").toString().toInt())
                                ->getCoordinates();
                        try {
                            holes.append(factory->createLinearRing(sequence));
                        } catch(geos::util::IllegalArgumentException e) {
                            qCritical() << e.what();
                        }
                    }

                } else if (reader.name().toString() == "nd") {
                    coordinates.append(*zipCodeNodes.value(reader.attributes()
                            .value("ref").toString().toInt())->getCoordinate());
                } else if (reader.name().toString() == "tag") {
                    if (reader.attributes().value("k") == "addr:postcode") {
                        polygon = true;
                        zipCode = reader.attributes().value("v").toString().toInt();
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "way") {

                    geos::geom::CoordinateSequence* sequence = factory
                                ->getCoordinateSequenceFactory()->create(
                                (std::vector<geos::geom::Coordinate>*) NULL, 2);
                    for (int i = 0; i < coordinates.size(); i++) {
                        sequence->add(coordinates.at(i));
                    }
                    if (polygon) {
                        geos::geom::LinearRing* ring = factory->createLinearRing(sequence->clone());
                        zipCodes.insert(zipCode, QSharedPointer<geos::geom::Polygon>(
                                            factory->createPolygon(ring, NULL)));
                        zipCode = 0;
                        polygon = false;
                    }

                    zipCodeWays.insert(wayId, factory->createLineString(sequence));
                    coordinates.clear();
                } else if (reader.name().toString() == "relation") {
                    geos::geom::LinearRing* outerRing = factory
                            ->createLinearRing(baseSequence);
                    std::vector<geos::geom::Geometry*> innerHoles = holes
                            .toVector().toStdVector();
                    zipCodes.insert(zipCode, QSharedPointer<geos::geom::Polygon>(
                                        factory->createPolygon(*outerRing, innerHoles)));

                    baseSequence = NULL;
                    delete outerRing;
                    holes.clear();
                }
                break;
            default:
                break;
        }
    }

    QList<geos::geom::Point*> zipCodeNodesPoints = zipCodeNodes.values();
    for (int i = 0; i < zipCodeNodesPoints.size(); i++) {
        delete zipCodeNodesPoints.at(i);
    }
    zipCodeNodes.clear();

    QList<geos::geom::LineString*> zipCodeWaysPoints = zipCodeWays.values();
    for (int i = 0; i < zipCodeWaysPoints.size(); i++) {
        delete zipCodeWaysPoints.at(i);
    }
    zipCodeWays.clear();

    if (logOptions & ZipCodesLoaded) {
        output = output % newline;
        output = output % QString("Zip Codes:") % newline;

        QList<int> zipCodeKeys = zipCodes.keys();
        for (int i = 0; i < zipCodes.size(); i++) {
            output = output % QString::number(zipCodeKeys.at(i)) % newline;
        }
    }

    readBuildingFile();
}

void FultonCountyConverter::readBuildingFile() {
    if (buildingsFile.fileName().isEmpty()) {
        readAddressFile();
        return;
    }

    if (!buildingsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Couldn't open " << buildingsFile.fileName();
        readAddressFile();
        return;
    }

    QHash<qlonglong, geos::geom::Point*> buildingNodes;
    QHash<qlonglong, geos::geom::LineString*> buildingWays;

    QXmlStreamReader reader(&buildingsFile);
    qlonglong wayId;
    QString featureId;
    bool skip1 = true;
    int year;
    QMap<QString, QString> tags;
    geos::geom::CoordinateSequence* baseSequence = NULL;
    QList<geos::geom::Geometry*> holes;
    QList<geos::geom::Coordinate> coordinates;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    geos::geom::Coordinate coordinate;
                    coordinate.y = reader.attributes().value("lat").toString().toDouble();
                    coordinate.x = reader.attributes().value("lon").toString().toDouble();
                    qlonglong nodeId = reader.attributes().value("id").toString().toLongLong();
                    buildingNodes.insert(nodeId, factory->createPoint(coordinate));
                } else if (reader.name().toString() == "way") {
                    wayId = reader.attributes().value("id").toString().toLongLong();
                } else if (reader.name().toString() == "member") {
                    if (reader.attributes().value("role").toString() == "outer") {
                        geos::geom::CoordinateSequence* sequence = buildingWays
                                .value(reader.attributes().value("ref").toString().toLongLong())
                                ->getCoordinates();
                        if (baseSequence == NULL) {
                            baseSequence = sequence;
                        } else {
                            if (sequence->size() > baseSequence->size()) {
                                baseSequence = sequence;
                            }
                        }
                    } else if (reader.attributes().value("role").toString() == "inner") {
                        geos::geom::CoordinateSequence* sequence = buildingWays
                                .value(reader.attributes().value("ref").toString().toLongLong())
                                ->getCoordinates();
                        try {
                            holes.append(factory->createLinearRing(sequence));
                        } catch(geos::util::IllegalArgumentException e) {
                            qCritical() << e.what();
                        }
                    }
                } else if (reader.name().toString() == "nd") {
                    coordinates.append(*buildingNodes.value(reader.attributes()
                            .value("ref").toString().toLongLong())->getCoordinate());
                } else if (reader.name().toString() == "tag") {
                    if (reader.attributes().value("k").toString() == "FeatureID") {
                        featureId = reader.attributes().value("v").toString();
                        skip1 = false;
                    } else if (reader.attributes().value("k").toString() == "YearBuilt") {
                        year = reader.attributes().value("v").toString().toInt();
                    } else if (reader.attributes().value("k").toString() == "FeatType") {
                        tags.insert("-featureType", reader.attributes().value("v").toString());
                    } else if (reader.attributes().value("k").toString() == "LUCDesc") {
                        if (reader.attributes().value("v").toString() == "Churches, Synogogue, Mosque") {
                            tags.insert("amenity", "place_of_worship");
                        } else if (reader.attributes().value("v").toString() == "Radio, TV or Motion Picture Studio") {
                            tags.insert("amenity", "studio");
                        } else if (reader.attributes().value("v").toString() == "Hospital") {
                            tags.insert("amenity", "hospital");
                        } else if (reader.attributes().value("v").toString() == "Parking Garage/Deck"
                                   || reader.attributes().value("v").toString() == "Parking Lot (Paved)") {
                            tags.insert("amenity", "parking");
                        } else if (reader.attributes().value("v").toString() == "Library") {
                            tags.insert("amenity", "library");
                        } else if (reader.attributes().value("v").contains("Office Bldg")) {
                            tags.insert("building", "commercial");
                        } else if (tags.value("-featureType") == "Residential"
                                   && reader.attributes().value("v").contains("Residential")) {
                            tags.insert("building", "house");
                        } else if (reader.attributes().value("v").startsWith("Apt")
                                   && !reader.attributes().value("v").contains("Garden")
                                   && !reader.attributes().value("v").contains("Retail")) {
                            tags.insert("building", "apartments");
                        } else if (reader.attributes().value("v").startsWith("Retail")
                                   || reader.attributes().value("v").contains("Shopping")) {
                            tags.insert("building", "retail");
                        }
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "way") {

                    geos::geom::CoordinateSequence* sequence = factory
                                ->getCoordinateSequenceFactory()->create(
                                (std::vector<geos::geom::Coordinate>*) NULL, 2);
                    bool skip = true;
                    for (int i = 0; i < coordinates.size(); i++) {
                        geos::geom::Coordinate coord = coordinates.at(i);
                        if (skip && coord.y >= bottom && coord.y <= top
                                && coord.x >= left && coord.x <= right) {
                            skip = false;
                        }
                        sequence->add(coord);
                    }

                    if (!skip && !skip1) {
                        geos::geom::LinearRing* ring = factory->createLinearRing(sequence->clone());
                        Building building;
                        building.setFeatureID(featureId);
                        building.setYear(year);
                        building.setTags(tags);
                        building.setBuilding(QSharedPointer<geos::geom::Polygon>
                            (factory->createPolygon(ring, NULL)));
                        buildings.append(building);

                        const geos::geom::Envelope* envelope = building.building()->getEnvelopeInternal();
                        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                        buildingsTree.insert(min, max, building);

                        delete[] min;
                        delete[] max;
                    }

                    skip1 = true;
                    tags.clear();

                    buildingWays.insert(wayId, factory->createLineString(sequence));
                    coordinates.clear();
                } else if (reader.name().toString() == "relation") {
                    geos::geom::LinearRing* outerRing = factory
                            ->createLinearRing(baseSequence);
                    std::vector<geos::geom::Geometry*> innerHoles = holes
                            .toVector().toStdVector();

                    bool skip = true;
                    for (std::size_t i = 0; i < baseSequence->size(); i++) {
                        const geos::geom::Coordinate coord = baseSequence->getAt(i);
                        if (coord.y >= bottom && coord.y <= top
                                && coord.x >= left && coord.x <= right) {
                            skip = false;
                            break;
                        }
                    }

                    if (!skip) {
                        Building building;
                        building.setFeatureID(featureId);
                        building.setYear(year);
                        building.setTags(tags);
                        building.setBuilding(QSharedPointer<geos::geom::Polygon>
                            (factory->createPolygon(*outerRing, innerHoles)));
                        buildings.append(building);

                        const geos::geom::Envelope* envelope = building.building()->getEnvelopeInternal();
                        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                        buildingsTree.insert(min, max, building);

                        delete[] min;
                        delete[] max;
                    }

                    delete outerRing;
                    baseSequence = NULL;
                    holes.clear();
                    tags.clear();
                }
                break;
            default:
                break;
        }
    }

    QList<geos::geom::Point*> buildingNodesPoints = buildingNodes.values();
    for (int i = 0; i < buildingNodesPoints.size(); i++) {
        delete buildingNodesPoints.at(i);
    }
    buildingNodes.clear();

    QList<geos::geom::LineString*> buildingWaysPoints = buildingWays.values();
    for (int i = 0; i < buildingWaysPoints.size(); i++) {
        delete buildingWaysPoints.at(i);
    }
    buildingWays.clear();

    validateBuildings();
}

void FultonCountyConverter::validateBuildings() {
    if (logOptions & OverlappingBuildings) {
        output = output % newline;
        output = output % QStringLiteral("Removed Overlapping Buildings: ") % newline;
    }
    for (int i = 0; i < buildings.size(); ++i) {
        const Building& building1 = buildings.at(i);

        geos::geom::prep::PreparedPolygon polygon(building1.building().data());

        const geos::geom::Envelope* envelope = building1.building()->getEnvelopeInternal();
        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

        QList<Building> nearbyBuildings = buildingsTree.search(min, max, NULL, NULL);

        for (int j = 0; j < nearbyBuildings.size(); ++j) {
            Building building2 = nearbyBuildings.at(j);
            if (building1 == building2) {
                continue;
            }

            if (polygon.intersects(building2.building().data())) {
                if (building1.year() >= building2.year()) {
                    if (logOptions & OverlappingBuildings) {
                        const geos::geom::Coordinate* centroid = building2.building()
                                ->getCentroid()->getCoordinate();
                        output = output % QString("(%1, %2)")
                                .arg(centroid->y).arg(centroid->x) % newline;
                    }

                    const geos::geom::Envelope* envelope2 = building2.building()->getEnvelopeInternal();
                    double* min2 = new double[2] {envelope2->getMinX(), envelope2->getMinY()};
                    double* max2 = new double[2] {envelope2->getMaxX(), envelope2->getMaxY()};

                    buildingsTree.remove(min, max, building2);
                    buildings.removeOne(building2);

                    delete[] min2;
                    delete[] max2;
                } else {
                    if (logOptions & OverlappingBuildings) {
                        const geos::geom::Coordinate* centroid = building1.building()
                                ->getCentroid()->getCoordinate();
                        output = output % QString("(%1, %2)")
                                .arg(centroid->y).arg(centroid->x) % newline;
                    }
                    buildingsTree.remove(min, max, building1);
                    buildings.removeOne(building1);
                    break;
                }
            }
        }

        delete[] min;
        delete[] max;
    }

    removeExistingIntersectingBuildings();
}

void FultonCountyConverter::removeExistingIntersectingBuildings() {
    for (int i = 0; i < existingBuildings.size(); i++) {
        const Building& existingBuilding = existingBuildings.at(i);

        geos::geom::prep::PreparedPolygon polygon(existingBuilding.building()
                .data());
        for (int j = 0; j < buildings.size(); j++) {
            Building building = buildings.at(j);

            if (polygon.intersects(building.building().data())) {
                buildings.removeAt(j);

                if (logOptions & OverlappingBuildings) {
                    const geos::geom::Coordinate* centroid = building.building()
                            ->getCentroid()->getCoordinate();
                    output = output % QString("(%1, %2) (existing building)")
                            .arg(centroid->y).arg(centroid->x) % newline;
                }

                const geos::geom::Envelope* envelope = building.building()->getEnvelopeInternal();
                double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                buildingsTree.remove(min, max, building);

                delete[] min;
                delete[] max;

                j--;
            }
        }
    }

    simplifyBuildings();
}

void FultonCountyConverter::simplifyBuildings() {
    for (int i = 0; i < buildings.size(); i++) {
        Building building = buildings.at(i);
        geos::geom::Polygon* polygon = building.building().data();

        if (polygon->getArea() * DEGREES_TO_METERS * DEGREES_TO_METERS < 10) {
            buildings.removeAt(i);
            i--;
            continue;
        }

        geos::geom::CoordinateSequence* coordinates = polygon->getExteriorRing()
                ->getCoordinates();

        for (std::size_t j = 0; j < coordinates->size() - 1; j++) {
            geos::geom::Coordinate current = coordinates->getAt(j);
            geos::geom::Coordinate after = coordinates->getAt(j + 1);

            if (current.distance(after) * DEGREES_TO_METERS < 0.05) {
                if (j = coordinates->size() - 2) {
                    // For some reason, removing at this case causes buildings to look weird.
                    //coordinates->deleteAt(j);
                } else {
                    coordinates->deleteAt(j + 1);
                }
            }
        }

        for (std::size_t j = 1; j < coordinates->size() - 1; j++) {
            geos::geom::Coordinate before = coordinates->getAt(j - 1);
            geos::geom::Coordinate current = coordinates->getAt(j);
            geos::geom::Coordinate after = coordinates->getAt(j + 1);

            double headingDiff = geos::algorithm::Angle::toDegrees(
                geos::algorithm::Angle::angleBetween(before,
                    current, after));

            // In the first case, the line is effectively going back on itself.
            // In the second case, it's effectively a straight line.
            if (headingDiff < 1 || qAbs(headingDiff - 180) < 5) {
                coordinates->deleteAt(j);
                j--;
            }
        }

        geos::geom::LinearRing* newLinearRing = factory->createLinearRing(coordinates);
        std::vector<geos::geom::Geometry*>* innerHoles = new std::vector<geos::geom::Geometry*>();
        innerHoles->reserve(polygon->getNumInteriorRing());
        for (std::size_t j = 0; j < polygon->getNumInteriorRing(); j++) {
            innerHoles->push_back(polygon->getInteriorRingN(j)->clone());
        }
        geos::geom::Polygon* newPolygon = factory->createPolygon(newLinearRing, innerHoles);
        building.setBuilding(QSharedPointer<geos::geom::Polygon>(newPolygon));
        buildings.replace(i, building);
    }

    readAddressFile();
}

void FultonCountyConverter::readAddressFile() {
    if (!addressesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Couldn't open " << addressesFile.fileName();
        cleanup();
        return;
    }

    if (logOptions & AddressesButNoStreet) {
        output = output % newline;
        output = output % QStringLiteral("Addresses in Area, But No Street") % newline;
    }

    QXmlStreamReader reader(&addressesFile);
    Address address;
    bool skip = false;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    address = Address();
                    geos::geom::Coordinate coordinate;
                    coordinate.y = reader.attributes().value("lat").toString().toDouble();
                    coordinate.x = reader.attributes().value("lon").toString().toDouble();
                    address.setCoordinate(QSharedPointer<geos::geom::Point>(factory->createPoint(coordinate)));
                    // Check to see if the address is inside the BBox
                    skip = !(coordinate.y <= top &&
                            coordinate.y >= bottom &&
                            coordinate.x >= left &&
                            coordinate.x <= right);
                } else if (reader.name().toString() == "tag" && !skip) {
                    if (reader.attributes().value("k") == "addr:housenumber") {
                        address.setHouseNumber(reader.attributes().value("v").toString());
                    } else if (reader.attributes().value("k") == "addr:street") {
                        QString streetName = reader.attributes().value("v").toString();
                        QList<Street> matches = streets.values(streetName.toUpper());
                        if (!matches.isEmpty()) {
                            Street closestStreet = matches.at(0);
                            double minDistance = address.coordinate()->distance(closestStreet.path().data());
                            for (int i = 1; i < matches.size(); i++) {
                                double distance = address.coordinate()->distance(matches.at(i).path().data());
                                if (distance < minDistance) {
                                    closestStreet = matches.at(i);
                                    minDistance = distance;
                                }
                            }
                            address.setStreet(closestStreet);
                        } else {
                            address.street().setName(toTitleCase(streetName));
                            if (logOptions & AddressesButNoStreet) {
                                output = output % QString(address.houseNumber() + " " + address.street().name()) % newline;
                            }
                            excludedAddresses.append(address);
                            skip = true;
                        }
                    } else if (reader.attributes().value("k") == "addr:city") {
                        address.setCity(toTitleCase(reader.attributes().value("v").toString()));
                    } else if (reader.attributes().value("k") == "addr:postcode") {
                        address.setZipCode(reader.attributes().value("v").toString().toInt());
                    } else if (reader.attributes().value("k") == "import:FEAT_TYPE") {
                        if (reader.attributes().value("v") == "driv") {
                            address.setAddressType(Address::Primary);
                        } else if (reader.attributes().value("v") == "stru") {
                            address.setAddressType(Address::Structural);
                        }
                    }
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "node") {
                    if (!skip) {
                        if (!address.houseNumber().isEmpty()
                                && !address.street().name().isEmpty()
                                && !existingAddresses.contains(address)
                                && address.addressType() != Address::Other) {
                            int i = newAddresses.indexOf(address);
                            if (i != -1) {
                                Address existingAddress = newAddresses.at(i);

                                const geos::geom::Envelope* envelope = existingAddress.coordinate()->getEnvelopeInternal();
                                double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                                double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                                addressesTree.remove(min, max, existingAddress);

                                delete[] min;
                                delete[] max;

                                if (existingAddress.addressType() == Address::Structural
                                        && address.addressType() == Address::Structural) {
                                    existingAddress.setAllowStructural(false);
                                } else if (existingAddress.addressType() == Address::Primary
                                        && address.addressType() == Address::Structural
                                        && existingAddress.allowStructural()) {
                                    existingAddress.setCoordinate(address.coordinate());
                                    existingAddress.setAddressType(Address::Structural);
                                } else if (existingAddress.addressType() == Address::Structural
                                        && address.addressType() == Address::Primary
                                        && !existingAddress.allowStructural()) {
                                    existingAddress.setCoordinate(address.coordinate());
                                    existingAddress.setAddressType(Address::Primary);
                                }

                                envelope = existingAddress.coordinate()->getEnvelopeInternal();
                                min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                                max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                                addressesTree.insert(min, max, existingAddress);

                                delete[] min;
                                delete[] max;

                                newAddresses.replace(i, existingAddress);
                            } else {
                                newAddresses.append(address);

                                const geos::geom::Envelope* envelope = address.coordinate()->getEnvelopeInternal();
                                double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                                double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                                addressesTree.insert(min, max, address);

                                delete[] min;
                                delete[] max;
                            }
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
    if (logOptions & NewAddressesBeforeValidation) {
        output = output % newline;
        output = output % QStringLiteral("New Addresses (before validation):") % newline;
        for (int i = 0; i < newAddresses.size(); i++) {
            Address address = newAddresses.at(i);
            output = output % (address.houseNumber() + " " + address.street().name()) % newline;
        }
    }
    validateAddresses();
}

void FultonCountyConverter::validateAddresses() {
    if (logOptions & (AddressesFarFromStreet | AddressesCloseToStreet | CloseAddresses)) {
        output = output % newline;
        output = output % QStringLiteral("Address Validations") % newline;
    }
    for (int i = 0; i < newAddresses.size(); i++) {
        Address address = newAddresses.at(i);

        double distance = address.coordinate()->distance(address.street().path().data()) * DEGREES_TO_METERS;

        if (distance > 100) {
            if (logOptions & AddressesFarFromStreet) {
                output = output % QString("Too far from the street: %1 %2").arg(address.houseNumber())
                        .arg(address.street().name()) % newline;
            }
            newAddresses.removeOne(address);

            const geos::geom::Envelope* envelope = address.coordinate()->getEnvelopeInternal();
            double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
            double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

            addressesTree.remove(min, max, address);

            excludedAddresses.append(address);
            i--;
        } else if (distance < 5) {
            if (logOptions & AddressesCloseToStreet) {
                output = output % QString("Too close to the street: %1 %2").arg(address.houseNumber())
                        .arg(address.street().name()) % newline;
            }
            newAddresses.removeOne(address);

            const geos::geom::Envelope* envelope = address.coordinate()->getEnvelopeInternal();
            double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
            double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

            addressesTree.remove(min, max, address);

            excludedAddresses.append(address);
            i--;
        }
    }
    validateBetweenAddresses();

    if (logOptions & NewAddressesAfterValidation) {
        output = output % newline;
        output = output % QStringLiteral("New Addresses (after validation):") % newline;
        for (int i = 0; i < newAddresses.size(); i++) {
            const Address& address = newAddresses.at(i);
            output = output % (address.houseNumber() + " " + address.street().name()) % newline;
        }
    }

    checkZipCodes();
}

void FultonCountyConverter::validateBetweenAddresses() {
    for (int i = 0; i < newAddresses.size(); i++) {
        const Address& address1 = newAddresses.at(i);

        geos::geom::Envelope envelope(*address1.coordinate()->getEnvelopeInternal());
        envelope.expandBy(3.0 / DEGREES_TO_METERS);
        double* min = new double[2] {envelope.getMinX(), envelope.getMinY()};
        double* max = new double[2] {envelope.getMaxX(), envelope.getMaxY()};

        QList<Address> addresses = addressesTree.search(min, max, NULL, NULL);

        for (int j = 0; j < addresses.size(); ++j) {
            Address address2 = addresses.at(j);

            if (address1 == address2) {
                continue;
            }

            double distance = address1.coordinate()->distance(address2.coordinate().data()) * DEGREES_TO_METERS;

            if (distance < 3) {
                if (logOptions & CloseAddresses) {
                    output = output % QString("Too close to another address: %1 %2")
                            .arg(address2.houseNumber()).arg(address2.street().name()) % newline;
                }
                newAddresses.removeOne(address2);

                const geos::geom::Envelope* envelope2 = address2.coordinate()->getEnvelopeInternal();
                double* min2 = new double[2] {envelope2->getMinX(), envelope2->getMinY()};
                double* max2 = new double[2] {envelope2->getMaxX(), envelope2->getMaxY()};

                addressesTree.remove(min2, max2, address2);

                excludedAddresses.append(address2);
            }
        }
    }
}

void FultonCountyConverter::checkZipCodes() {
    if (zipCodes.size() == 0) {
        readTaxParcels();
        return;
    }

    if (logOptions & AddressesNotInZipCodeArea) {
        output = output % newline;
        output = output % QStringLiteral("Addresses not in zipcodes:") % newline;
    }

    for (int i = 0; i < newAddresses.size(); i++) {
        Address address = newAddresses[i];
        geos::geom::Polygon* zipCodePolygon = zipCodes.value(address.zipCode(), QSharedPointer<geos::geom::Polygon>()).data();

        if (zipCodePolygon == NULL || !zipCodePolygon->contains(address.coordinate().data())) {
            if (logOptions & AddressesNotInZipCodeArea) {
                output = output % QString("%1 %2, %3").arg(address.houseNumber())
                        .arg(address.street().name()).arg(address.zipCode()) % newline;
            }
            address.setZipCode(0);
            newAddresses[i] = address;
        }
    }

    readTaxParcels();
}

void FultonCountyConverter::readTaxParcels() {
    if (taxParcelsFile.fileName().isEmpty()) {
        mergeAddressBuildingNearby();
        return;
    }

    if (!taxParcelsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Couldn't open " << taxParcelsFile.fileName();
        mergeAddressBuildingNearby();
        return;
    }

    QHash<qlonglong, geos::geom::Point*> taxParcelsNodes;
    QHash<qlonglong, geos::geom::LineString*> taxParcelsWays;

    QXmlStreamReader reader(&taxParcelsFile);
    qlonglong wayId;
    QString featureId;
    geos::geom::CoordinateSequence* baseSequence = NULL;
    QList<geos::geom::Geometry*> holes;
    QList<geos::geom::Coordinate> coordinates;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (reader.name().toString() == "node") {
                    geos::geom::Coordinate coordinate;
                    coordinate.y = reader.attributes().value("lat").toString().toDouble();
                    coordinate.x = reader.attributes().value("lon").toString().toDouble();
                    qlonglong nodeId = reader.attributes().value("id").toString().toLongLong();
                    taxParcelsNodes.insert(nodeId, factory->createPoint(coordinate));
                } else if (reader.name().toString() == "way") {
                    wayId = reader.attributes().value("id").toString().toLongLong();
                } else if (reader.name().toString() == "member") {
                    if (reader.attributes().value("role").toString() == "outer") {
                        geos::geom::CoordinateSequence* sequence = taxParcelsWays
                                .value(reader.attributes().value("ref").toString().toLongLong())
                                ->getCoordinates();
                        if (baseSequence == NULL) {
                            baseSequence = sequence;
                        } else {
                            if (sequence->size() > baseSequence->size()) {
                                baseSequence = sequence;
                            }
                        }
                    } else if (reader.attributes().value("role").toString() == "inner") {
                        geos::geom::CoordinateSequence* sequence = taxParcelsWays
                                .value(reader.attributes().value("ref").toString().toLongLong())
                                ->getCoordinates();
                        try {
                            holes.append(factory->createLinearRing(sequence));
                        } catch(geos::util::IllegalArgumentException e) {
                            qCritical() << e.what();
                        }
                    }
                } else if (reader.name().toString() == "nd") {
                    coordinates.append(*taxParcelsNodes.value(reader.attributes()
                            .value("ref").toString().toLongLong())->getCoordinate());
                }
                break;
            case QXmlStreamReader::EndElement:
                if (reader.name().toString() == "way") {

                    geos::geom::CoordinateSequence* sequence = factory
                                ->getCoordinateSequenceFactory()->create(
                                (std::vector<geos::geom::Coordinate>*) NULL, 2);
                    bool skip = true;
                    for (int i = 0; i < coordinates.size(); i++) {
                        geos::geom::Coordinate coord = coordinates.at(i);
                        if (skip && coord.y >= bottom && coord.y <= top
                                && coord.x >= left && coord.x <= right) {
                            skip = false;
                        }
                        sequence->add(coord);
                    }

                    if (!skip) {
                        geos::geom::LinearRing* ring = factory->createLinearRing(sequence->clone());
                        QSharedPointer<geos::geom::Polygon> polygon = QSharedPointer<geos::geom::Polygon>
                                (factory->createPolygon(ring, NULL));
                        taxParcels.append(polygon);

                        const geos::geom::Envelope* envelope = polygon->getEnvelopeInternal();
                        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                        taxParcelsTree.insert(min, max, polygon);

                        delete[] min;
                        delete[] max;
                    }

                    taxParcelsWays.insert(wayId, factory->createLineString(sequence));
                    coordinates.clear();
                } else if (reader.name().toString() == "relation") {
                    geos::geom::LinearRing* outerRing = factory
                            ->createLinearRing(baseSequence);
                    std::vector<geos::geom::Geometry*> innerHoles = holes
                            .toVector().toStdVector();

                    bool skip = true;
                    for (std::size_t i = 0; i < baseSequence->size(); i++) {
                        const geos::geom::Coordinate coord = baseSequence->getAt(i);
                        if (coord.y >= bottom && coord.y <= top
                                && coord.x >= left && coord.x <= right) {
                            skip = false;
                            break;
                        }
                    }

                    if (!skip) {
                        QSharedPointer<geos::geom::Polygon> polygon = QSharedPointer<geos::geom::Polygon>
                                (factory->createPolygon(*outerRing, innerHoles));
                        taxParcels.append(polygon);

                        const geos::geom::Envelope* envelope = polygon->getEnvelopeInternal();
                        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
                        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

                        taxParcelsTree.insert(min, max, polygon);

                        delete[] min;
                        delete[] max;
                    }

                    delete outerRing;
                    baseSequence = NULL;
                    holes.clear();
                }
                break;
            default:
                break;
        }
    }

    mergeAddressBuildingWithin();
}

void FultonCountyConverter::mergeAddressBuildingWithin() {
    if (logOptions & MergedAddressesAndBuilding) {
        output = output % newline;
        output = output % QStringLiteral("Contained addresses merged into buildings") % newline;
    }

    for (int i = 0; i < (buildings.size() + existingBuildings.size()); i++) {
        Building building = i < buildings.size()
                ? buildings.at(i)
                : existingBuildings.at(i - buildings.size());

        bool unaddressedBuilding = true;
        QList<QString> keys = building.tags().keys();
        for (int j = 0; j < keys.size(); ++j) {
            if (keys.at(j).startsWith("addr:")) {
                unaddressedBuilding = false;
                break;
            }
        }
        if (!unaddressedBuilding) {
            continue;
        }

        const geos::geom::Envelope* envelope = building.building()->getEnvelopeInternal();

        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

        QList<Address> innerAddresses = addressesTree.search(min, max, NULL, NULL);

        delete[] min;
        delete[] max;

        bool hasInnerAddress = false;
        Address innerAddress;
        for (int j = 0; j < innerAddresses.size(); ++j) {
            Address address = innerAddresses.at(j);
            if (addressBuildings.contains(address)) {
                continue;
            }
            if (building.building()->contains(address.coordinate().data())) {
                if (!hasInnerAddress) {
                    hasInnerAddress = true;
                    innerAddress = address;
                } else {
                    hasInnerAddress = false;
                    break;
                }
            }
        }

        if (hasInnerAddress) {
            building.setMergeLevel(Building::Within);
            addressBuildings.insert(innerAddress, building);
        }
    }

    QList<Address> mergedAddresses = addressBuildings.keys();
    for (int i = 0; i < mergedAddresses.size(); i++) {
        Address mergedAddress = mergedAddresses.at(i);
        if (newAddresses.removeOne(mergedAddress)
                && (logOptions & MergedAddressesAndBuilding)) {
            output = output % QString("%1 %2").arg(mergedAddress.houseNumber(),
                    mergedAddress.street().name()) % newline;
        }
    }

    QList<Building> mergedBuildings = addressBuildings.values();
    for (int i = 0; i < mergedBuildings.size(); i++) {
        buildings.removeOne(mergedBuildings.at(i));
        existingBuildings.removeOne(mergedBuildings.at(i));
    }

    mergeAddressBuildingTaxParcels();
}

void FultonCountyConverter::mergeAddressBuildingTaxParcels() {
    if (logOptions & MergedAddressesAndBuilding) {
        output = output % newline;
        output = output % QStringLiteral("Addresses merged into buildings from tax parcel data") % newline;
    }

    for (int i = 0; i < taxParcels.size(); ++i) {
        QSharedPointer<geos::geom::Polygon> taxParcel = taxParcels.at(i);

        const geos::geom::Envelope* envelope = taxParcel->getEnvelopeInternal();

        double* min = new double[2] {envelope->getMinX(), envelope->getMinY()};
        double* max = new double[2] {envelope->getMaxX(), envelope->getMaxY()};

        QList<Address> nearAddresses = addressesTree.search(min, max, NULL, NULL);
        QList<Building> nearBuildings = buildingsTree.search(min, max, NULL, NULL);

        int numInnerAddresses = 0;
        Address innerAddress;
        int numInnerBuildings = 0;
        Building innerBuilding;

        for (int j = 0; j < nearAddresses.size(); ++j) {
            Address address = nearAddresses.at(j);

            if (taxParcel->contains(address.coordinate().data())) {
                ++numInnerAddresses;
                innerAddress = address;
                if (numInnerAddresses > 1) {
                    break;
                }
            }
        }

        if (numInnerAddresses > 1) {
            continue;
        }

        for (int j = 0; j < nearBuildings.size(); ++j) {
            Building building = nearBuildings.at(j);

            if (taxParcel->contains(building.building().data())) {
                if (numInnerBuildings == 0) {
                    innerBuilding = building;
                    numInnerBuildings++;
                } else {
                    numInnerBuildings++;
                    double area1 = innerBuilding.building()->getArea();
                    double area2 = building.building()->getArea();
                    if (area2 > area1) {
                        innerBuilding = building;
                    }
                }
            }
        }

        if (numInnerAddresses == 1 && numInnerBuildings > 0) {
            innerBuilding.setMergeLevel(Building::TaxParcel);
            addressBuildings.insert(innerAddress, innerBuilding);
        }
    }

    QList<Address> mergedAddresses = addressBuildings.keys();
    for (int i = 0; i < mergedAddresses.size(); i++) {
        Address mergedAddress = mergedAddresses.at(i);
        if (newAddresses.removeOne(mergedAddress)
                && (logOptions & MergedAddressesAndBuilding)) {
            output = output % QString("%1 %2").arg(mergedAddress.houseNumber(),
                    mergedAddress.street().name()) % newline;
        }
    }

    QList<Building> mergedBuildings = addressBuildings.values();
    for (int i = 0; i < mergedBuildings.size(); i++) {
        buildings.removeOne(mergedBuildings.at(i));
        existingBuildings.removeOne(mergedBuildings.at(i));
    }

    mergeAddressBuildingNearby();
}

void FultonCountyConverter::mergeAddressBuildingNearby() {
    if (logOptions & MergedAddressesAndBuilding) {
        output = output % newline;
        output = output % QStringLiteral("Nearby addresses merged into buildings") % newline;
    }

    for (int i = 0; i < (buildings.size() + existingBuildings.size()); i++) {
        Building building = i < buildings.size()
                ? buildings.at(i)
                : existingBuildings.at(i - buildings.size());

        bool unaddressedBuilding = true;
        QList<QString> keys = building.tags().keys();
        for (int j = 0; j < keys.size(); ++j) {
            if (keys.at(j).startsWith("addr:")) {
                unaddressedBuilding = false;
                break;
            }
        }
        if (!unaddressedBuilding) {
            continue;
        }

        geos::geom::Envelope envelope = geos::geom::Envelope(*building.building()->getEnvelopeInternal());
        envelope.expandBy(20.0 / DEGREES_TO_METERS);

        double* min = new double[2] {envelope.getMinX(), envelope.getMinY()};
        double* max = new double[2] {envelope.getMaxX(), envelope.getMaxY()};

        QList<Address> innerAddresses = addressesTree.search(min, max, NULL, NULL);

        delete[] min;
        delete[] max;

        bool hasBestAddress = false;
        Address bestAddress;
        double maxDistance = 25;
        for (int j = 0; j < innerAddresses.size(); ++j) {
            Address address = innerAddresses.at(j);
            if (building.building()->contains(address.coordinate().data())) {
                hasBestAddress = false;
                break;
            }

            double distance = building.building()->distance(
                        address.coordinate().data()) * DEGREES_TO_METERS;
            if (distance < maxDistance) {
                maxDistance = distance;
                hasBestAddress = true;
                bestAddress = address;
            }
        }

        if (hasBestAddress) {
            if (addressBuildings.contains(bestAddress)) {
                Building addedBuilding = addressBuildings.value(bestAddress);
                double distance = addedBuilding.building()->distance(
                            bestAddress.coordinate().data()) * DEGREES_TO_METERS;
                if (addedBuilding.mergeLevel() == Building::Nearby && distance < maxDistance) {
                    if (addedBuilding.id() != 0) {
                        existingBuildings.append(addedBuilding);
                    } else {
                        buildings.append(addedBuilding);
                    }
                    addedBuilding.setMergeLevel(Building::Unmerged);
                    addressBuildings.remove(bestAddress);
                    building.setMergeLevel(Building::Nearby);
                    addressBuildings.insert(bestAddress, building);
                }
            } else {
                addressBuildings.insert(bestAddress, building);
            }
        }
    }

    QList<Address> mergedAddresses = addressBuildings.keys();
    for (int i = 0; i < mergedAddresses.size(); i++) {
        Address mergedAddress = mergedAddresses.at(i);
        if (newAddresses.removeOne(mergedAddress)
                && (logOptions & MergedAddressesAndBuilding)) {
            output = output % QString("%1 %2").arg(mergedAddress.houseNumber(),
                    mergedAddress.street().name()) % newline;
        }
    }

    QList<Building> mergedBuildings = addressBuildings.values();
    for (int i = 0; i < mergedBuildings.size(); i++) {
        buildings.removeOne(mergedBuildings.at(i));
        existingBuildings.removeOne(mergedBuildings.at(i));
    }

    emit conversionFinished();
}

void FultonCountyConverter::writeChangeFile(QString fileName) {
    if (fileName.isEmpty()) {
        return;
    }
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        writeXMLFile(file, newAddresses, buildings, addressBuildings);
    }

    QString fullFileName;
    for (int i = 0; i <= excludedAddresses.size() / 5000; i++) {
        if (i == 0) {
            if (fileName.lastIndexOf(".") == -1) {
                fullFileName = QString("%1_errors").arg(fileName);
            }
            else {
                QString baseName = fileName.left(fileName.lastIndexOf("."));
                QString extension = fileName.right(fileName.length() - fileName.lastIndexOf(".") - 1);
                fullFileName = QString("%1_errors.%2").arg(baseName).arg(extension);
            }
        } else {
            if (fileName.lastIndexOf(".") == -1) {
                fullFileName = QString("%1_errors_%2").arg(fileName).arg(i);
            }
            else {
                QString baseName = fileName.left(fileName.lastIndexOf("."));
                QString extension = fileName.right(fileName.length() - fileName.lastIndexOf(".") - 1);
                fullFileName = QString("%1_errors_%2.%3").arg(baseName).arg(i).arg(extension);
            }
        }
        QFile file(fullFileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            continue;
        }

        writeXMLFile(file, excludedAddresses, QList<Building>(), QHash<Address, Building>());
    }

    // Output the log to a file
    QString baseName = fileName.left(fileName.lastIndexOf("."));
    QFile logFile(baseName + ".log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream writer(&logFile);
        writer << output;
        writer.flush();
    }

    cleanup();
}

void FultonCountyConverter::writeXMLFile(QFile& file, const QList<Address> addresses,
        const QList<Building> buildings, const QHash<Address, Building> addressBuildings) {
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    outputStartOfFile(writer);
    writer.writeStartElement("create");
    uint id = 1;

    for (int i = 0; i < addresses.size(); i++) {
        Address address = addresses.at(i);

        writer.writeStartElement("node");
        writer.writeAttribute("id", QString("-%1").arg(id));
        id++;
        writer.writeAttribute("lat", QString::number(address.coordinate()->getY(), 'g', 12));
        writer.writeAttribute("lon", QString::number(address.coordinate()->getX(), 'g', 12));

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:housenumber");
        writer.writeAttribute("v", address.houseNumber());
        writer.writeEndElement();

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:street");
        writer.writeAttribute("v", address.street().name());
        writer.writeEndElement();

        if (!address.city().isEmpty()) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:city");
            writer.writeAttribute("v", address.city());
            writer.writeEndElement();
        }

        if (address.zipCode() != 0) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:postcode");
            writer.writeAttribute("v", QString::number(address.zipCode()));
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    for (int i = 0; i < buildings.size(); i++) {
        Building building = buildings.at(i);

        const geos::geom::CoordinateSequence* coordinates = building.building()
                ->getExteriorRing()->getCoordinatesRO();
        for (std::size_t j = 0; j < coordinates->size() - 1; j++) {
            geos::geom::Coordinate coordinate = coordinates->getAt(j);
            writer.writeStartElement("node");
            writer.writeAttribute("id", QString("-%1").arg(id));
            id++;
            writer.writeAttribute("lat", QString::number(coordinate.y, 'g', 12));
            writer.writeAttribute("lon", QString::number(coordinate.x, 'g', 12));
            writer.writeEndElement();
        }

        writer.writeStartElement("way");
        writer.writeAttribute("id", QString("-%1").arg(id));
        int wayId = id;
        id++;

        for (int j = coordinates->size() - 1; j >= 1; j--) {
            writer.writeStartElement("nd");
            writer.writeAttribute("ref", QString("-%1").arg(id - (j + 1)));
            writer.writeEndElement();
        }
        writer.writeStartElement("nd");
        writer.writeAttribute("ref", QString("-%1").arg(id - coordinates->size()));
        writer.writeEndElement();

        std::size_t numHoles = building.building()->getNumInteriorRing();
        if (numHoles > 0) {
            writer.writeEndElement();

            QList<int> innerWayIds;

            for (std::size_t j = 0; j < numHoles; j++) {
                const geos::geom::CoordinateSequence* innerCoordinates = building
                    .building()->getInteriorRingN(j)->getCoordinatesRO();

                for (std::size_t k = 0; k < innerCoordinates->size() - 1; k++) {
                    geos::geom::Coordinate coordinate = innerCoordinates->getAt(k);
                    writer.writeStartElement("node");
                    writer.writeAttribute("id", QString("-%1").arg(id));
                    id++;
                    writer.writeAttribute("lat", QString::number(coordinate.y, 'g', 12));
                    writer.writeAttribute("lon", QString::number(coordinate.x, 'g', 12));
                    writer.writeEndElement();
                }

                writer.writeStartElement("way");
                writer.writeAttribute("id", QString("-%1").arg(id));
                innerWayIds.append(id);
                id++;

                for (int k = innerCoordinates->size() - 1; k >= 1; k--) {
                    writer.writeStartElement("nd");
                    writer.writeAttribute("ref", QString("-%1").arg(id - (k + 1)));
                    writer.writeEndElement();
                }
                writer.writeStartElement("nd");
                writer.writeAttribute("ref", QString("-%1").arg(id - innerCoordinates->size()));
                writer.writeEndElement();

                writer.writeEndElement();
            }

            writer.writeStartElement("relation");
            writer.writeAttribute("id", QString("-%1").arg(id));
            id++;

            writer.writeStartElement("member");
            writer.writeAttribute("type", "way");
            writer.writeAttribute("ref", QString("-%1").arg(wayId));
            writer.writeAttribute("role", "outer");
            writer.writeEndElement();

            for (int j = 0; j < innerWayIds.size(); j++) {
                writer.writeStartElement("member");
                writer.writeAttribute("type", "way");
                writer.writeAttribute("ref", QString("-%1").arg(innerWayIds.at(j)));
                writer.writeAttribute("role", "inner");
                writer.writeEndElement();
            }

            writer.writeStartElement("tag");
            writer.writeAttribute("k", "type");
            writer.writeAttribute("v", "multipolygon");
            writer.writeEndElement();
        }

        QList<QString> keys = building.tags().keys();
        for (int j = 0; j < building.tags().size(); ++j) {
            if (keys.at(j).startsWith("-")) {
                continue;
            }
            writer.writeStartElement("tag");
            writer.writeAttribute("k", keys.at(j));
            writer.writeAttribute("v", building.tags().value(keys.at(j)));
            writer.writeEndElement();
        }

        if (!building.tags().contains("building")) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "building");
            writer.writeAttribute("v", "yes");
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    QList<Address> mergedAddresses = addressBuildings.keys();
    QList<Building> mergedBuildings = addressBuildings.values();
    for (int i = 0; i < mergedBuildings.size(); i++) {
        Address address = mergedAddresses.at(i);
        Building building = mergedBuildings.at(i);

        if (building.id() != 0) {
            continue;
        }

        const geos::geom::CoordinateSequence* coordinates = building.building()
                ->getExteriorRing()->getCoordinatesRO();
        for (std::size_t j = 0; j < coordinates->size() - 1; j++) {
            geos::geom::Coordinate coordinate = coordinates->getAt(j);
            writer.writeStartElement("node");
            writer.writeAttribute("id", QString("-%1").arg(id));
            id++;
            writer.writeAttribute("lat", QString::number(coordinate.y, 'g', 12));
            writer.writeAttribute("lon", QString::number(coordinate.x, 'g', 12));
            writer.writeEndElement();
        }

        writer.writeStartElement("way");
        writer.writeAttribute("id", QString("-%1").arg(id));
        int wayId = id;
        id++;

        for (int j = coordinates->size() - 1; j >= 1; j--) {
            writer.writeStartElement("nd");
            writer.writeAttribute("ref", QString("-%1").arg(id - (j + 1)));
            writer.writeEndElement();
        }
        writer.writeStartElement("nd");
        writer.writeAttribute("ref", QString("-%1").arg(id - coordinates->size()));
        writer.writeEndElement();

        std::size_t numHoles = building.building()->getNumInteriorRing();
        if (numHoles > 0) {
            writer.writeEndElement();

            QList<int> innerWayIds;

            for (std::size_t j = 0; j < numHoles; j++) {
                const geos::geom::CoordinateSequence* innerCoordinates = building
                    .building()->getInteriorRingN(j)->getCoordinatesRO();

                for (std::size_t k = 0; k < innerCoordinates->size() - 1; k++) {
                    geos::geom::Coordinate coordinate = innerCoordinates->getAt(k);
                    writer.writeStartElement("node");
                    writer.writeAttribute("id", QString("-%1").arg(id));
                    id++;
                    writer.writeAttribute("lat", QString::number(coordinate.y, 'g', 12));
                    writer.writeAttribute("lon", QString::number(coordinate.x, 'g', 12));
                    writer.writeEndElement();
                }

                writer.writeStartElement("way");
                writer.writeAttribute("id", QString("-%1").arg(id));
                innerWayIds.append(id);
                id++;

                for (int k = innerCoordinates->size() - 1; k >= 1; k--) {
                    writer.writeStartElement("nd");
                    writer.writeAttribute("ref", QString("-%1").arg(id - (k + 1)));
                    writer.writeEndElement();
                }
                writer.writeStartElement("nd");
                writer.writeAttribute("ref", QString("-%1").arg(id - innerCoordinates->size()));
                writer.writeEndElement();

                writer.writeEndElement();
            }

            writer.writeStartElement("relation");
            writer.writeAttribute("id", QString("-%1").arg(id));
            id++;

            writer.writeStartElement("member");
            writer.writeAttribute("type", "way");
            writer.writeAttribute("ref", QString("-%1").arg(wayId));
            writer.writeAttribute("role", "outer");
            writer.writeEndElement();

            for (int j = 0; j < innerWayIds.size(); j++) {
                writer.writeStartElement("member");
                writer.writeAttribute("type", "way");
                writer.writeAttribute("ref", QString("-%1").arg(innerWayIds.at(j)));
                writer.writeAttribute("role", "inner");
                writer.writeEndElement();
            }

            writer.writeStartElement("tag");
            writer.writeAttribute("k", "type");
            writer.writeAttribute("v", "multipolygon");
            writer.writeEndElement();
        }

        QList<QString> keys = building.tags().keys();
        for (int j = 0; j < building.tags().size(); ++j) {
            if (keys.at(j).startsWith("-")) {
                continue;
            }
            writer.writeStartElement("tag");
            writer.writeAttribute("k", keys.at(j));
            writer.writeAttribute("v", building.tags().value(keys.at(j)));
            writer.writeEndElement();
        }

        if (!building.tags().contains("building")) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "building");
            writer.writeAttribute("v", "yes");
            writer.writeEndElement();
        }

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:housenumber");
        writer.writeAttribute("v", address.houseNumber());
        writer.writeEndElement();

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:street");
        writer.writeAttribute("v", address.street().name());
        writer.writeEndElement();

        if (!address.city().isEmpty()) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:city");
            writer.writeAttribute("v", address.city());
            writer.writeEndElement();
        }

        if (address.zipCode() != 0) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:postcode");
            writer.writeAttribute("v", QString::number(address.zipCode()));
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeStartElement("modify");

    for (int i = 0; i < mergedBuildings.size(); i++) {
        Address address = mergedAddresses.at(i);
        Building building = mergedBuildings.at(i);

        if (building.id() == 0) {
            continue;
        }

        const geos::geom::CoordinateSequence* coordinates = building.building()
                ->getExteriorRing()->getCoordinatesRO();
        for (std::size_t j = 0; j < coordinates->size() - 1; j++) {
            geos::geom::Coordinate coordinate = coordinates->getAt(j);
            writer.writeStartElement("node");
            writer.writeAttribute("id", QString::number(building.nodeIndices().at(j)));
            writer.writeAttribute("version", QString::number(nodes.value(building.nodeIndices().at(j)).version));
            writer.writeAttribute("lat", QString::number(coordinate.y, 'g', 12));
            writer.writeAttribute("lon", QString::number(coordinate.x, 'g', 12));
            writer.writeAttribute("user", building.user());
            writer.writeAttribute("uid", QString::number(building.uid()));
            writer.writeAttribute("changeset", QString::number(building.changesetID()));
            writer.writeAttribute("timestamp", building.timestamp().toString(Qt::ISODate));
            writer.writeEndElement();
        }

        writer.writeStartElement("way");
        writer.writeAttribute("id", QString::number(building.id()));
        writer.writeAttribute("version", QString::number(building.version()));
        writer.writeAttribute("user", building.user());
        writer.writeAttribute("uid", QString::number(building.uid()));
        writer.writeAttribute("changeset", QString::number(building.changesetID()));
        writer.writeAttribute("timestamp", building.timestamp().toString(Qt::ISODate));

        for (int j = 0; j < building.nodeIndices().size(); j++) {
            writer.writeStartElement("nd");
            writer.writeAttribute("ref", QString::number(building.nodeIndices().at(j)));
            writer.writeEndElement();
        }

        QList<QString> keys = building.tags().keys();
        for (int j = 0; j < building.tags().size(); ++j) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", keys.at(j));
            writer.writeAttribute("v", building.tags().value(keys.at(j)));
            writer.writeEndElement();
        }

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:housenumber");
        writer.writeAttribute("v", address.houseNumber());
        writer.writeEndElement();

        writer.writeStartElement("tag");
        writer.writeAttribute("k", "addr:street");
        writer.writeAttribute("v", address.street().name());
        writer.writeEndElement();

        if (!address.city().isEmpty()) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:city");
            writer.writeAttribute("v", address.city());
            writer.writeEndElement();
        }

        if (address.zipCode() != 0) {
            writer.writeStartElement("tag");
            writer.writeAttribute("k", "addr:postcode");
            writer.writeAttribute("v", QString::number(address.zipCode()));
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();
    outputEndOfFile(writer);
}

void FultonCountyConverter::outputStartOfFile(QXmlStreamWriter& writer) {
    writer.writeStartDocument();
    writer.writeStartElement("osmChange");
    writer.writeAttribute("version", "0.6");
}

void FultonCountyConverter::outputEndOfFile(QXmlStreamWriter& writer) {
    writer.writeEndElement();
    writer.writeEndDocument();
}

void FultonCountyConverter::cleanup() {
    nodes.clear();
    streets.clear();
    existingAddresses.clear();
    newAddresses.clear();
    excludedAddresses.clear();
    addressBuildings.clear();
    zipCodes.clear();
    addressesTree.removeAll();
    buildingsTree.removeAll();
}

QString FultonCountyConverter::expandQuadrant(QString street) {
    return street.replace("NE", "Northeast").replace("NW", "Northwest")
            .replace("SE", "Southeast").replace("SW", "Southwest");
}

QString FultonCountyConverter::toTitleCase(QString str) {
    QRegExp re("\\W\\w");
    int pos = -1;
    str = str.toLower();
    QChar *base = str.data();
    QChar *ch;
    do {
        pos++;
        ch = base + pos;
        pos = str.indexOf(re, pos);
        *ch = ch->toUpper();
    } while (pos >= 0);
    return str;
}

FultonCountyConverter::Logging operator| (FultonCountyConverter::Logging a, FultonCountyConverter::Logging b)
{
    typedef std::underlying_type<FultonCountyConverter::Logging>::type UL;
    return FultonCountyConverter::Logging(static_cast<UL>(a) | static_cast<UL>(b));
}

FultonCountyConverter::~FultonCountyConverter() {
    delete nam;
    delete factory;
}
