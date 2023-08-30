#!/bin/bash
set -e

AWS_ACCOUNT_NUM=$AWS_ACCOUNT_NUM
AWS_REGION=$AWS_ACCOUNT_NUM
DEV_IOT_THING=$DEV_IOT_THING
DEV_IOT_THING_GROUP=$DEV_IOT_THING_GROUP

if [ -x "$(command -v jq)" ]; then
    echo "JQ already installed"
else
    echo "Installing JQ"
    sudo apt-get install jq
fi

EDGEML_ZIP="edgeml-accelerator-package.zip"
if test -f "$EDGEML_ZIP"; then
    echo "$EDGEML_ZIP exists."
    cp $EDGEML_ZIP com.aws.edgeml.accelerator/greengrass/.
else
    echo "$EDGEML_ZIP does not exist. Exiting..."
    exit
fi

if [ -x "$(command -v gdk)" ]; then
    echo "GDK already installed"
else
    echo "Installing GDK"
    python3 -m pip install -U git+https://github.com/aws-greengrass/aws-greengrass-gdk-cli.git@v1.2.0
fi

# Setup IoT device
THING_ARN="arn:aws:iot:${AWS_REGION}:${AWS_ACCOUNT_NUM}:thing/${DEV_IOT_THING}"

# Automate the revision updates for GDK Build and Publish
pushd com.aws.edgeml.accelerator/greengrass
if [ ! -f revision ]
then
    echo 1 > revision
fi
REVISION_VER=$(cat revision)
NEXT_REV=$((REVISION_VER+1))
echo $NEXT_REV > revision
echo Revision Version: $REVISION_VER

# Set up greengrass component version
export VERSION=$(cat version)
COMPLETE_VER="$VERSION.$REVISION_VER"
VER=${COMPLETE_VER}

echo Building Component Version: $VER

jq -r --arg VER "$VER" '.component[].version=$VER' gdk-config.json > gdk-config.json.bak
mv gdk-config.json.bak gdk-config.json
jq -r --arg AWS_REGION "$AWS_REGION" '.component[].publish.region=$AWS_REGION' gdk-config.json > gdk-config.json.bak
mv gdk-config.json.bak gdk-config.json

COMPONENTCONFIGURATION=$(jq -r '.ComponentConfiguration.DefaultConfiguration' recipe.json)

# GDK Build and GDK Publish
echo Building GDK component
gdk component build
echo Publishing GDK component
gdk component publish

# GDK Component Deployment
jq -r --arg VER "$VER" '.components[].componentVersion=$VER' deployment-config.json > deployment-config.json.bak
mv deployment-config.json.bak deployment-config.json
jq -r --arg THING_ARN "$THING_ARN" '.targetArn=$THING_ARN' deployment-config.json > deployment-config.json.bak
mv deployment-config.json.bak deployment-config.json
jq -r --arg DEV_IOT_THING_GROUP "$DEV_IOT_THING_GROUP" '.deploymentName=$DEV_IOT_THING_GROUP' deployment-config.json > deployment-config.json.bak
mv deployment-config.json.bak deployment-config.json
jq -r --arg COMPONENTCONFIGURATION "$COMPONENTCONFIGURATION" '.components[].configurationUpdate.merge=$COMPONENTCONFIGURATION' deployment-config.json > deployment-config.json.bak
mv deployment-config.json.bak deployment-config.json

CONFIG_FILE="deployment-config.json"
RES=`aws greengrassv2 create-deployment --target-arn $THING_ARN --cli-input-json fileb://$CONFIG_FILE --region $AWS_REGION`
echo Greengrass Deployment ID: ${RES}
popd