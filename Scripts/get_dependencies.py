#
#   This file is a part of Jiayin's Graphics Samples.
#   Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
#

# This is a helper script to get dependencies

import os
import urllib.request
import zipfile
import shutil
import sys

# whether to force syncing
forcing_sync = False

if len(sys.argv) > 1:
    # output a message indicating this is a force syncing
    print( 'Force syncing dependencies.' )

    if sys.argv[1] == 'TRUE':
        forcing_sync = sys.argv[0]

# dependencies folder
dep_dir = 'Dependencies'

# whether to sync dependencies
sync_dep = False

# if forcing syncing is enabled, delete the dependencies even if it exists
if forcing_sync:
    # check if the folder already exists, if it does, remove it
    if os.path.isdir(dep_dir):
        # output a warning
        print('The dependencies are purged.')

        # remove the folder
        shutil.rmtree(dep_dir)

    # re-create the folder again
    os.makedirs(dep_dir)

    sync_dep = True
else:
    # this might not be very robust since it just check the folder
    # if there is a broken dependencies folder, it will fail to build
    if os.path.isdir(dep_dir) is False:
        sync_dep = True
    
# sync dependencies if needed
if sync_dep:
    # acquire the mini vulkan sdk
    url = 'http://45.63.123.194/vulkan_sdk/VulkanSDK_1_2_148_0.zip'
    zip_file_name = 'vulkan_sdk_tmp.zip'
    local_filename, dummy = urllib.request.urlretrieve(url, zip_file_name)

    # uncompress the zip file and make sure it is in the Dependencies folder
    with zipfile.ZipFile(zip_file_name,"r") as zip_ref:
        zip_ref.extractall('Dependencies')

    # delete the temporary file
    os.remove(zip_file_name)
